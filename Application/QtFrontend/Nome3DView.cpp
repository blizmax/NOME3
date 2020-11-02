#include "Nome3DView.h"
#include "FrontendContext.h"
#include "MainWindow.h"
#include <Scene/Mesh.h>

#include <QDialog>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStatusBar>
#include <QTableWidget>

namespace Nome
{

CNome3DView::CNome3DView()
{
    Root = new Qt3DCore::QEntity();
    this->setRootEntity(Root);
    // MakeGridEntity(Root); Removing grid entity per Professor Sequin's request

    // Make a point light
    auto* lightEntity = new Qt3DCore::QEntity(Root);
    auto* light = new Qt3DRender::QPointLight(lightEntity);
    light->setColor("white");
    light->setIntensity(1);
    lightEntity->addComponent(light);
    auto* lightTransform = new Qt3DCore::QTransform(lightEntity);
    lightTransform->setTranslation({ 100.0f, 100.0f, 100.0f });
    lightEntity->addComponent(lightTransform);

    // Tweak render settings
    this->defaultFrameGraph()->setClearColor(QColor(QRgb(0x4d4d4f)));

    // Setup camera
    // TODO: aspect ratio
    auto* camera = this->camera();
    camera->lens()->setPerspectiveProjection(45.0f, 1280.f / 720.f, 0.1f, 1000.0f);
    camera->setPosition(QVector3D(0, 0, 40.0f));
    camera->setViewCenter(QVector3D(0, 0, 0));

    auto* camController = new Qt3DExtras::QOrbitCameraController(Root);
    camController->setLinearSpeed(50.0f);
    camController->setLookSpeed(180.0f);
    camController->setCamera(camera);
}

CNome3DView::~CNome3DView() { UnloadScene(); }

void CNome3DView::TakeScene(const tc::TAutoPtr<Scene::CScene>& scene)
{
    using namespace Scene;
    Scene = scene;
    Scene->Update();
    Scene->ForEachSceneTreeNode([this](CSceneTreeNode* node) {
        printf("%s\n", node->GetPath().c_str());
        auto* entity = node->GetInstanceEntity();
        if (!entity)
        {
            entity = node->GetOwner()->GetEntity();
        }

        if (entity)
        {
            printf("    %s\n", entity->GetName().c_str());

            // Create an InteractiveMesh from the scene node
            auto* mesh = new CInteractiveMesh(node);
            mesh->setParent(this->Root);
            InteractiveMeshes.insert(mesh);
        }
    });
    PostSceneUpdate();
}

void CNome3DView::UnloadScene()
{
    for (auto* m : InteractiveMeshes)
        delete m;
    InteractiveMeshes.clear();
    Scene = nullptr;
}



void CNome3DView::PostSceneUpdate()
{
    using namespace Scene;
    std::unordered_map<CSceneTreeNode*, CInteractiveMesh*> sceneNodeAssoc;
    std::unordered_set<CInteractiveMesh*> aliveSet;
    std::unordered_map<Scene::CEntity*, CDebugDraw*> aliveEntityDrawData;
    for (auto* m : InteractiveMeshes)
        sceneNodeAssoc.emplace(m->GetSceneTreeNode(), m);

    Scene->ForEachSceneTreeNode([&](CSceneTreeNode* node) {
        // Obtain either an instance entity or a shared entity from the scene node
        auto* entity = node->GetInstanceEntity();
        if (!entity)
        {
            entity = node->GetOwner()->GetEntity();
        }

        if (entity)
        {
            CInteractiveMesh* mesh = nullptr;
            // Check for existing InteractiveMesh
            auto iter = sceneNodeAssoc.find(node);
            if (iter != sceneNodeAssoc.end())
            {
                // Found existing InteractiveMesh, mark as alive
                mesh = iter->second;
                aliveSet.insert(mesh);
                mesh->UpdateTransform();
                if (node->WasEntityUpdated())
                {
                    printf("Geom regen for %s\n", node->GetPath().c_str());
                    
                    mesh->CreateInteractiveFaces(); // Randy added this even alter 10/12 afternoon. Noticed needed for subdivisoin. USE. This was accidently left commented on 10/14, was causing bugs on 10/15
                    if (WireFrameMode)
                        mesh->UpdateFaceGeometries(true); // Randy added this on 10/12. USE
                    else
                        mesh->UpdateFaceGeometries(false);
                    mesh->UpdatePointGeometries(); // Randy added this on 10/12 afternoon. Fixed point coloring issue. USE
                    //mesh->UpdateFaceMaterials(); // Randy added this on 10/12. DONT USE
                    mesh->InitInteractions(); // Randy added 10/15 night. wait this didnt fix the crash bug... do we still need it?
                    //TODO: Default should be below, because it's faster. Enable a "Face selection" button that makes it use the above
                    //mesh->UpdateGeometry(); // optimize this to work in the future. right now we can't have two materials on the same face unfortunately 
                    //mesh->UpdateMaterial(); // optimize this to wokr in the future. right now we can't have two materials on the same face unfortunately
                    node->SetEntityUpdated(false);
                    std::cout << "FINISHed updating everything" << std::endl;
                }
            }
            else
            {
                mesh = new CInteractiveMesh(node);
                mesh->setParent(this->Root);
                aliveSet.insert(mesh);
                InteractiveMeshes.insert(mesh);
            }
            // Create a DebugDraw for the CEntity if not already
            auto eIter = EntityDrawData.find(entity);
            if (eIter == EntityDrawData.end())
            {
                auto* debugDraw = new CDebugDraw(Root);
                aliveEntityDrawData[entity] = debugDraw;
                // TODO: somehow uncommenting this line leads to a crash in Qt3D
                // mesh->SetDebugDraw(debugDraw);
            }
            else
            {
                aliveEntityDrawData[entity] = eIter->second;
                mesh->SetDebugDraw(eIter->second);
            }
        }
    });
 
    // Now kill all the dead objects, i.e., not longer in the scene graph. If it wasn't added to aliveset above, then it is dead.
    for (auto* m : InteractiveMeshes)
    {
        auto iter = aliveSet.find(m);
        if (iter == aliveSet.end())
        {
            // Not in aliveSet
            delete m;
        }
    }
    InteractiveMeshes = std::move(aliveSet);

    

    // Kill all entity debug draws that are not alive
    for (auto& iter : EntityDrawData)
    {
        auto iter2 = aliveEntityDrawData.find(iter.first);
        if (iter2 == aliveEntityDrawData.end())
        {
            delete iter.second;
        }
    }

    EntityDrawData = std::move(aliveEntityDrawData);
    for (const auto& pair : EntityDrawData)
    {
        pair.second->Reset();
        pair.first->Draw(pair.second);
        pair.second->Commit();
    }
}

// Randy added 9/27
void CNome3DView::ClearSelectedVertices()
{
    SelectedVertices.clear();
    Scene->ForEachSceneTreeNode([&](Scene::CSceneTreeNode* node) {
        // Obtain either an instance entity or a shared entity from the scene node
        auto* entity = node->GetInstanceEntity();
        if (!entity)
            entity = node->GetOwner()->GetEntity();
        if (entity)
        {
            auto* meshInst = dynamic_cast<Scene::CMeshInstance*>(entity);
            meshInst->DeselectAll();
        }
    });
}

// Randy added on 10/14 to clear face selection
void CNome3DView::ClearSelectedFaces()
{
    SelectedFaces.clear();
    Scene->ForEachSceneTreeNode([&](Scene::CSceneTreeNode* node) {
        // Obtain either an instance entity or a shared entity from the scene node
        auto* entity = node->GetInstanceEntity();
        if (!entity)
            entity = node->GetOwner()->GetEntity();
        if (entity)
        {
            auto* meshInst = dynamic_cast<Scene::CMeshInstance*>(entity);
            meshInst->DeselectAll();
        }
    });
}

void CNome3DView::PickFaceWorldRay(const tc::Ray& ray)
{

    std::vector<std::tuple<float, Scene::CMeshInstance*, std::string>> hits;
    Scene->ForEachSceneTreeNode([&](Scene::CSceneTreeNode* node) {
        std::cout << "Currently in NOME3DView's PickFaceWorldRay. At node: " + node->GetPath() << std::endl;
        // Obtain either an instance entity or a shared entity from the scene node
        auto* entity = node->GetInstanceEntity();
        if (!entity)
            entity = node->GetOwner()->GetEntity();
        if (entity)
        {
            const auto& l2w = node->L2WTransform.GetValue(tc::Matrix3x4::IDENTITY);
            auto localRay = ray.Transformed(l2w.Inverse());
            localRay.Direction =localRay.Direction.Normalized(); // Normalize to fix "scale" error caused by l2w.Inverse()
            auto* meshInst = dynamic_cast<Scene::CMeshInstance*>(entity);
            auto pickResults = meshInst->PickFaces(localRay);
            for (const auto& [dist, name] : pickResults)
                hits.emplace_back(dist, meshInst, name);
        }
    });

    std::sort(hits.begin(), hits.end());
    //if (!hits.empty()) {
    //    hits.resize(1); // Force there to be only one face selected. This is more user-friendly.
    //}
    if (hits.size() == 1)
    {
        const auto& [dist, meshInst, faceName] = hits[0];
        std::vector<std::string>::iterator position =
            std::find(SelectedFaces.begin(), SelectedFaces.end(), faceName);
        if (position == SelectedFaces.end())
        { // if this face has not been selected before
            SelectedFaces.push_back(faceName); // add face to selected faces
            GFrtCtx->MainWindow->statusBar()->showMessage(
                QString::fromStdString("Selected " + faceName));
        }
        else // else, this face has been selected previously
        {
            SelectedFaces.erase(position);
            GFrtCtx->MainWindow->statusBar()->showMessage(
                QString::fromStdString("Unselected " + faceName));
        }
        meshInst->MarkFaceAsSelected({ faceName }, true);
    }
    else if (!hits.empty())
    {
        // Show a dialog for the user to choose one face
        auto* dialog = new QDialog(GFrtCtx->MainWindow);
        dialog->setModal(true);
        auto* layout1 = new QHBoxLayout(dialog);
        auto* table = new QTableWidget();
        table->setRowCount(hits.size());
        table->setColumnCount(2);
        QStringList titles;
        titles.append(QString::fromStdString("Closeness Rank"));
        titles.append(QString::fromStdString("Face Name"));
        table->setHorizontalHeaderLabels(titles);
        int closenessRank = 1;
        for (size_t i = 0; i < hits.size(); i++)
        {
            const auto& [dist, meshInst, faceName] = hits[i];
            if (i != 0)
            {
                const auto& [prevDist, prevMeshInst, prevFaceName] = hits[i - 1];
                if (round(dist * 100) != round(prevDist * 100))
                {
                    closenessRank += 1;
                }
                // else, closenessRank stay the same as prev as the distance is the same (faces
                // in same location)
            }

            auto* distWidget = new QTableWidgetItem(QString::number(closenessRank));
            auto* item = new QTableWidgetItem(QString::fromStdString(faceName));
            table->setItem(i, 0, distWidget); // i is row num, and 0 is col num
            table->setItem(i, 1, item);
        }
        layout1->addWidget(table);
        auto* layout2 = new QVBoxLayout();
        auto* btnOk = new QPushButton();
        btnOk->setText("OK");
        connect(btnOk, &QPushButton::clicked, [this, dialog, table, hits]() {
            auto sel = table->selectedItems();
            if (!sel.empty())
            {
                int row = sel[0]->row();
                const auto& [dist, meshInst, faceName] = hits[row];
                std::vector<std::string>::iterator position =
                    std::find(SelectedFaces.begin(), SelectedFaces.end(), faceName);
                if (position == SelectedFaces.end())
                { // if this face has not been selected before
                    SelectedFaces.push_back(faceName); // add face to selected face
                    GFrtCtx->MainWindow->statusBar()->showMessage(
                        QString::fromStdString("Selected " + faceName));
                }
                else // else, this face has been selected previously
                {
                    SelectedFaces.erase(position);
                    GFrtCtx->MainWindow->statusBar()->showMessage(
                        QString::fromStdString("Unselected " + faceName));
                }

                float selected_dist = round(dist * 100);
                // mark all those that share the same location
                for (int i = 0; i < hits.size(); i++)
                {
                    const auto& [dist, meshInst, overlapfaceName] = hits[i];
                    if (round(dist * 100) == selected_dist)
                    {
                        meshInst->MarkFaceAsSelected({ overlapfaceName }, true);
                    }
                }
            }
            dialog->close();
        });
        auto* btnCancel = new QPushButton();
        btnCancel->setText("Cancel");
        connect(btnCancel, &QPushButton::clicked, dialog, &QWidget::close);
        layout2->addWidget(btnOk);
        layout2->addWidget(btnCancel);
        layout1->addLayout(layout2);
        dialog->show();
    }
    else
    {

        GFrtCtx->MainWindow->statusBar()->showMessage("No face hit.");
    }
}



// Used for picking edges
void CNome3DView::PickEdgeWorldRay(const tc::Ray& ray)
{

    std::vector<std::tuple<float, Scene::CMeshInstance*, std::vector<std::string>>> hits; // note the string is a vector of strings
    Scene->ForEachSceneTreeNode([&](Scene::CSceneTreeNode* node) {
        std::cout << "Currently in NOME3DView's PickEdgeWorldRay. At node: " + node->GetPath()
                  << std::endl;
        // Obtain either an instance entity or a shared entity from the scene node
        auto* entity = node->GetInstanceEntity();
        if (!entity)
            entity = node->GetOwner()->GetEntity();
        if (entity)
        {
            const auto& l2w = node->L2WTransform.GetValue(tc::Matrix3x4::IDENTITY);
            auto localRay = ray.Transformed(l2w.Inverse());
            localRay.Direction =
                localRay.Direction
                    .Normalized(); // Normalize to fix "scale" error caused by l2w.Inverse()
            auto* meshInst = dynamic_cast<Scene::CMeshInstance*>(entity);
            auto pickResults = meshInst->PickEdges(localRay);
            std::cout << "got edge pick results" << std::endl;
            std::cout << pickResults.size() << std::endl;
            for (const auto& [dist, names] : pickResults)
                hits.emplace_back(dist, meshInst, names);
        }
    });
    std::cout << "begin marking1" << std::endl;
    std::sort(hits.begin(), hits.end());
    // if (!hits.empty()) {
    //    hits.resize(1); // Force there to be only one face selected. This is more user-friendly.
    //}
    std::cout << "begin marking2" << std::endl;
    if (hits.size() == 1)
    {
        std::cout << "begin marking3" << std::endl;
        const auto& [dist, meshInst, edgeVertNames] = hits[0]; // where the edgeVertNames is defined to a vector of two vertex names
        std::vector<std::string>::iterator position1 =
            std::find(SelectedVertices.begin(), SelectedVertices.end(), edgeVertNames[0]);
        std::vector<std::string>::iterator position2 =
            std::find(SelectedVertices.begin(), SelectedVertices.end(), edgeVertNames[1]);
        
        std::cout << "begin marking4" << std::endl;
        if (position1 == SelectedVertices.end() || position2 == SelectedVertices.end()) // if either vertex has not been selected before, then the edge hasn't been selected
        { // if this edge has not been selected before
            std::cout << "begin marking5" << std::endl;
            SelectedVertices.push_back(edgeVertNames[0]);
            std::cout << "begin marking6" << std::endl;
            SelectedVertices.push_back(edgeVertNames[1]);
            std::cout << "begin marking7" << std::endl;
            GFrtCtx->MainWindow->statusBar()->showMessage(
                QString::fromStdString("Selected " + edgeVertNames[0] + edgeVertNames[1] + " edge"));
            std::cout << "begin marking8" << std::endl;
        }
        else // else, this edge has been selected previously
        {
         /*   std::cout << "tamada1" << std::endl;
            std::string removed;
            if (position1 != SelectedVertices.end())
            {
                std::cout << "tamada2" << std::endl;
                SelectedVertices.erase(position1);
                removed += edgeVertNames[0];
            }
            if (position2 != SelectedVertices.end())
            {
                std::cout << "tamad3" << std::endl;
                SelectedVertices.erase(position2);
                removed += edgeVertNames[1];
            }
            std::cout << "tamad4" << std::endl*/;
            GFrtCtx->MainWindow->statusBar()->showMessage(
                QString::fromStdString("TODO: Unselecte the edge"));
        }
        //meshInst->MarkEdgeAsSelected({ edgeVertNames }, true);
        std::set<std::string> edgeVertNamesSet(edgeVertNames.begin(), edgeVertNames.end());
        std::cout << "found edge, mark its vertices now" << std::endl;
        meshInst->MarkAsSelected(edgeVertNamesSet, true);
    }
    else if (!hits.empty())
    {
        // Show a dialog for the user to choose one face
        auto* dialog = new QDialog(GFrtCtx->MainWindow);
        dialog->setModal(true);
        auto* layout1 = new QHBoxLayout(dialog);
        auto* table = new QTableWidget();
        table->setRowCount(hits.size());
        table->setColumnCount(2);
        QStringList titles;
        titles.append(QString::fromStdString("Closeness Rank"));
        titles.append(QString::fromStdString("Edge Vert Names"));
        table->setHorizontalHeaderLabels(titles);
        int closenessRank = 1;
        for (size_t i = 0; i < hits.size(); i++)
        {
            const auto& [dist, meshInst, edgeVertNames] = hits[i];
            if (i != 0)
            {
                const auto& [prevDist, prevMeshInst, prevedgeVertNames] = hits[i - 1];
                if (round(dist * 100) != round(prevDist * 100))
                {
                    closenessRank += 1;
                }
                // else, closenessRank stay the same as prev as the distance is the same (faces
                // in same location)
            }

            auto* distWidget = new QTableWidgetItem(QString::number(closenessRank));
            auto* item = new QTableWidgetItem(QString::fromStdString(edgeVertNames[0] + " and " + edgeVertNames[1]));
            table->setItem(i, 0, distWidget); // i is row num, and 0 is col num
            table->setItem(i, 1, item);
        }
        layout1->addWidget(table);
        auto* layout2 = new QVBoxLayout();
        auto* btnOk = new QPushButton();
        btnOk->setText("OK");
        connect(btnOk, &QPushButton::clicked, [this, dialog, table, hits]() {
            auto sel = table->selectedItems();
            if (!sel.empty())
            {
                int row = sel[0]->row();
                const auto& [dist, meshInst, edgeVertNames] = hits[row];
                std::vector<std::string>::iterator position1 =
                    std::find(SelectedVertices.begin(), SelectedVertices.end(), edgeVertNames[0]);
                std::vector<std::string>::iterator position2 =
                    std::find(SelectedVertices.begin(), SelectedVertices.end(), edgeVertNames[1]);
                if (position1 == SelectedVertices.end() || position2 == SelectedVertices.end())
                { // if this face has not been selected before
                    SelectedVertices.push_back(edgeVertNames[0]);
                    SelectedVertices.push_back(edgeVertNames[1]);
                    GFrtCtx->MainWindow->statusBar()->showMessage(QString::fromStdString(
                        "Selected " + edgeVertNames[0] + edgeVertNames[1] + " edge"));
                }
                else // else, this face has been selected previously
                {
        /*            std::cout << "tamad1" << std::endl;
                    std::string removed;
                    if (position1 != SelectedVertices.end())
                    {
                        std::cout << "tamad2" << std::endl;
                        SelectedVertices.erase(position1);
                        removed += edgeVertNames[0];
                    }
                    if (position2 != SelectedVertices.end())
                    {
                        std::cout << "tamad3" << std::endl;
                        SelectedVertices.erase(position2);
                        removed += edgeVertNames[1];
                    }
                    std::cout << "tamad4" << std::endl;*/
                    GFrtCtx->MainWindow->statusBar()->showMessage(
                        QString::fromStdString("TODO: Unselect the  edge"));
                }

                float selected_dist = round(dist * 100);
                // mark all those that share the same location
                for (int i = 0; i < hits.size(); i++)
                {
                    const auto& [dist, meshInst, overlapedgeVertNames] = hits[i];
                    if (round(dist * 100) == selected_dist)
                    {

                        // temporary solution, add a polyline in that position in the future
                        //meshInst->MarkEdgeAsSelected({ overlapedgeVertNames }, true);
                        std::set<std::string> edgeVertNamesSet(edgeVertNames.begin(),
                                                               edgeVertNames.end());
                        meshInst->MarkAsSelected(edgeVertNamesSet, true); // mark the two edge vertices as selected
             
                    }
                }
            }
            dialog->close();
        });
        auto* btnCancel = new QPushButton();
        btnCancel->setText("Cancel");
        connect(btnCancel, &QPushButton::clicked, dialog, &QWidget::close);
        layout2->addWidget(btnOk);
        layout2->addWidget(btnCancel);
        layout1->addLayout(layout2);
        dialog->show();
    }
    else
    {

        GFrtCtx->MainWindow->statusBar()->showMessage("No edge hit.");
    }
}


void CNome3DView::PickVertexWorldRay(const tc::Ray& ray)
{

    std::vector<std::tuple<float, Scene::CMeshInstance*, std::string>> hits;
    Scene->ForEachSceneTreeNode([&](Scene::CSceneTreeNode* node) {
        std::cout << "in for each scene tree node for pickvertexworldray" << std::endl;
        std::cout << node->GetOwner()->GetName() << std::endl;
        // Obtain either an instance entity or a shared entity from the scene node
        auto* entity = node->GetInstanceEntity();
       // std::cout << "1A" << std::endl;
        if (!entity)
            entity = node->GetOwner()->GetEntity();
      //  std::cout << "2A" << std::endl;
        if (entity)
        {
           // std::cout << "3A" << std::endl;
            const auto& l2w = node->L2WTransform.GetValue(tc::Matrix3x4::IDENTITY);
            auto localRay = ray.Transformed(l2w.Inverse());
            localRay.Direction =
                localRay.Direction
                    .Normalized(); // Normalize to fix "scale" error caused by l2w.Inverse()
           // std::cout << "4A" << std::endl;
            auto* meshInst = dynamic_cast<Scene::CMeshInstance*>(entity);
          //  std::cout << "4.5A" << std::endl;
            auto pickResults = meshInst->PickVertices(localRay);
          //  std::cout << "5A" << std::endl;
            for (const auto& [dist, name] : pickResults)
                hits.emplace_back(dist, meshInst, name);
        }
    });

    std::sort(hits.begin(), hits.end());

    if (hits.size() == 1) 
    {
        const auto& [dist, meshInst, vertName] = hits[0];
        std::vector<std::string>::iterator position =
            std::find(SelectedVertices.begin(), SelectedVertices.end(), vertName);
        if (position == SelectedVertices.end())
        { // if this vertex has not been selected before
            std::cout << "gilber" + vertName << std::endl;
            SelectedVertices.push_back(vertName); // add vertex to selected vertices
            GFrtCtx->MainWindow->statusBar()->showMessage(
                QString::fromStdString("Selected " + vertName));
        }
        else // else, this vertex has been selected previously
        {
            SelectedVertices.erase(position);
            GFrtCtx->MainWindow->statusBar()->showMessage(
                QString::fromStdString("Unselected " + vertName));
        }
        meshInst->MarkAsSelected({ vertName }, true);
    }
    else if (!hits.empty())
    {
        // Show a dialog for the user to choose one vertex
        auto* dialog = new QDialog(GFrtCtx->MainWindow);
        dialog->setModal(true);
        auto* layout1 = new QHBoxLayout(dialog);
        auto* table = new QTableWidget();
        table->setRowCount(hits.size());
        table->setColumnCount(2);
        QStringList titles;
        titles.append(QString::fromStdString("Closeness Rank"));
        titles.append(QString::fromStdString("Vertex Name"));
        table->setHorizontalHeaderLabels(titles);
        int closenessRank = 1;
        for (size_t i = 0; i < hits.size(); i++)
        {
            const auto& [dist, meshInst, vertName] = hits[i];
            if (i != 0)
            {
                const auto& [prevDist, prevMeshInst, prevVertName] = hits[i - 1];
                if (round(dist * 100) != round(prevDist * 100))
                {
                    closenessRank += 1;
                }
                // else, closenessRank stay the same as prev as the distance is the same (vertices
                // in same location)
            }

            auto* distWidget = new QTableWidgetItem(QString::number(closenessRank));
            auto* item = new QTableWidgetItem(QString::fromStdString(vertName));
            table->setItem(i, 0, distWidget); // i is row num, and 0 is col num
            table->setItem(i, 1, item);
        }
        layout1->addWidget(table);
        auto* layout2 = new QVBoxLayout();
        auto* btnOk = new QPushButton();
        btnOk->setText("OK");
        connect(btnOk, &QPushButton::clicked, [this, dialog, table, hits]() {
            auto sel = table->selectedItems();
            if (!sel.empty())
            {
                int row = sel[0]->row();
                const auto& [dist, meshInst, vertName] = hits[row];
                std::vector<std::string>::iterator position =
                    std::find(SelectedVertices.begin(), SelectedVertices.end(), vertName);
                if (position == SelectedVertices.end())
                { // if this vertex has not been selected before
                    SelectedVertices.push_back(vertName); // add vertex to selected vertices
                    GFrtCtx->MainWindow->statusBar()->showMessage(
                        QString::fromStdString("Selected " + vertName));
                }
                else // else, this vertex has been selected previously
                {
                    SelectedVertices.erase(position);
                    GFrtCtx->MainWindow->statusBar()->showMessage(QString::fromStdString("Unselected " + vertName));
                }

                float selected_dist = round(dist * 100);

                // mark all those that share the same location
                for (int i = 0; i < hits.size(); i++)
                {
                    const auto& [dist, meshInst, overlapvertName] = hits[i];
                    if (round(dist * 100) == selected_dist)
                    {
                        meshInst->MarkAsSelected({ overlapvertName }, true);
                    }
                }
            }
            dialog->close();
        });
        auto* btnCancel = new QPushButton();
        btnCancel->setText("Cancel");
        connect(btnCancel, &QPushButton::clicked, dialog, &QWidget::close);
        layout2->addWidget(btnOk);
        layout2->addWidget(btnCancel);
        layout1->addLayout(layout2);
        dialog->show();
    }
    else
    {

        GFrtCtx->MainWindow->statusBar()->showMessage("No point hit.");
    }
}

// Currently not used
Qt3DCore::QEntity* CNome3DView::MakeGridEntity(Qt3DCore::QEntity* parent)
{
    const float size = 100.0f;
    const int divisions = 100;
    const float halfSize = size / 2.0f;

    auto* gridEntity = new Qt3DCore::QEntity(parent);

    auto* geometry = new Qt3DRender::QGeometry(gridEntity);

    QByteArray bufferBytes;
    bufferBytes.resize(3 * sizeof(float) * 4 * (divisions + 1));
    auto* positions = reinterpret_cast<float*>(bufferBytes.data());
    float xStart = -size / 2.0f;
    float increment = size / divisions;
    for (int i = 0; i <= divisions; i++)
    {
        *positions++ = xStart;
        *positions++ = 0.0f;
        *positions++ = -halfSize;
        *positions++ = xStart;
        *positions++ = 0.0f;
        *positions++ = halfSize;

        *positions++ = -halfSize;
        *positions++ = 0.0f;
        *positions++ = xStart;
        *positions++ = halfSize;
        *positions++ = 0.0f;
        *positions++ = xStart;
        xStart += increment;
    }

    auto* buf = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer, geometry);
    buf->setData(bufferBytes);

    auto* positionAttr = new Qt3DRender::QAttribute(geometry);
    positionAttr->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());
    positionAttr->setVertexBaseType(Qt3DRender::QAttribute::Float);
    positionAttr->setVertexSize(3);
    positionAttr->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    positionAttr->setBuffer(buf);
    positionAttr->setByteStride(3 * sizeof(float));
    positionAttr->setCount(4 * (divisions + 1));
    geometry->addAttribute(positionAttr);

    auto* gridMesh = new Qt3DRender::QGeometryRenderer(gridEntity);
    gridMesh->setGeometry(geometry);
    gridMesh->setPrimitiveType(Qt3DRender::QGeometryRenderer::Lines);
    auto* material = new Qt3DExtras::QPhongMaterial(gridEntity);
    material->setAmbient({ 255, 255, 255 });

    gridEntity->addComponent(gridMesh);
    gridEntity->addComponent(material);

    return gridEntity;
}

}