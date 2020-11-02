#include "FaceToQGeometry.h"
#include <iostream>
#include <Qt3DRender/QBuffer>

namespace NomeFace // Randy changed this
{

CFaceToQGeometry::CFaceToQGeometry(const CMeshImpl& fromMesh,
                                   const CMeshImpl::FaceHandle& fH, bool bGenPointGeometry)
{
    // Per face normal, thus no shared vertices between faces
    struct CVertexData
    {
        std::array<float, 3> Pos;
        std::array<float, 3> Normal;

        void SendToBuilder(CGeometryBuilder& builder) const // Randy note: I think here it's just decomposing it into a list of triangles with the correct normals
        {
            builder.Ingest(Pos[0], Pos[1], Pos[2]);
            builder.Ingest(Normal[0], Normal[1], Normal[2]);
        }
    };
    const uint32_t stride = sizeof(CVertexData);
    static_assert(stride == 24, "Vertex data size isn't as expected");
    QByteArray bufferArray;
    CAttribute attrPos { bufferArray, offsetof(CVertexData, Pos), stride,
                         Qt3DRender::QAttribute::Float, 3 };
    CAttribute attrNor { bufferArray, offsetof(CVertexData, Normal), stride,
                         Qt3DRender::QAttribute::Float, 3 };
    CGeometryBuilder builder;
    builder.AddAttribute(&attrPos);
    builder.AddAttribute(&attrNor);


    CVertexData v0, vPrev, vCurr;
    int faceVCount = 0;
    CMeshImpl::FaceVertexIter fvIter = CMeshImpl::FaceVertexIter(fromMesh, fH);
   //std::cout << "CHECK IS VALID" << std::endl;
    for (; fvIter.is_valid(); ++fvIter)
    {
        //std::cout << "FACE VERTEX IS VALID" << std::endl;
        CMeshImpl::VertexHandle faceVert = *fvIter;
        if (faceVCount == 0)
        {
            const auto& posVec = fromMesh.point(faceVert);
            v0.Pos = { posVec[0], posVec[1], posVec[2] };
            const auto& fnVec = fromMesh.normal(fH);
            v0.Normal = { fnVec[0], fnVec[1], fnVec[2] };
        }
        else if (faceVCount == 1)
        {
            const auto& posVec = fromMesh.point(faceVert);
            vPrev.Pos = { posVec[0], posVec[1], posVec[2] };
            const auto& fnVec = fromMesh.normal(fH);
            vPrev.Normal = { fnVec[0], fnVec[1], fnVec[2] };
        }
        else
        {
            const auto& posVec = fromMesh.point(faceVert);
            vCurr.Pos = { posVec[0], posVec[1], posVec[2] };
            const auto& fnVec = fromMesh.normal(fH);
            vCurr.Normal = { fnVec[0], fnVec[1], fnVec[2] };
            v0.SendToBuilder(builder);
            vPrev.SendToBuilder(builder);
            vCurr.SendToBuilder(builder);
            vPrev = vCurr;
        }
        faceVCount++;
    }
    
    Geometry = new Qt3DRender::QGeometry();

    auto* buffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer, Geometry);
    buffer->setData(bufferArray);

    auto* posAttr = new Qt3DRender::QAttribute(Geometry);
    posAttr->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());
    posAttr->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    posAttr->setBuffer(buffer);
    std::cout << "BUILDER VERTEX COUNT: " + std::to_string(builder.GetVertexCount()) << std::endl;
    posAttr->setCount(builder.GetVertexCount());
    attrPos.FillInQAttribute(posAttr);
    Geometry->addAttribute(posAttr);

    auto* normAttr = new Qt3DRender::QAttribute(Geometry);
    normAttr->setName(Qt3DRender::QAttribute::defaultNormalAttributeName());
    normAttr->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    normAttr->setBuffer(buffer);
    normAttr->setCount(builder.GetVertexCount());
    attrNor.FillInQAttribute(normAttr);
    Geometry->addAttribute(normAttr);

    if (bGenPointGeometry)
    {
        PointGeometry = new Qt3DRender::QGeometry();

        std::vector<float> pointBufferData;
        uint32_t vertexCount = 0;

        // Randy this may be a bug. we don't pointgeometry for every single face. That would take freaking forever.
        // this is how the vertex is displayed
        for (const auto& v : fromMesh.vertices())
        {
            const auto& point = fromMesh.point(v);
            const auto& color = fromMesh.color(v);
            pointBufferData.push_back(point[0]);
            pointBufferData.push_back(point[1]);
            pointBufferData.push_back(point[2]);
            // TODO: if selected, change color to something else
            pointBufferData.push_back(color[0] / 255.0f);
            pointBufferData.push_back(color[1] / 255.0f);
            pointBufferData.push_back(color[2] / 255.0f);
            printf("v%d: %d %d %d\n", vertexCount, color[0], color[1], color[2]);
            std::cout << "IN HERE" << std::endl;
            vertexCount++;
        }
        std::cout << "OUT HERE" << std::endl;
        QByteArray copyOfBuffer { reinterpret_cast<const char*>(pointBufferData.data()),
                                  static_cast<int>(pointBufferData.size() * sizeof(float)) };
        auto* buffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer, PointGeometry);
        buffer->setData(copyOfBuffer);

        posAttr = new Qt3DRender::QAttribute(PointGeometry);
        posAttr->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());
        posAttr->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
        posAttr->setBuffer(buffer);
        posAttr->setCount(vertexCount);
        posAttr->setByteOffset(0);
        posAttr->setByteStride(24);
        posAttr->setVertexBaseType(Qt3DRender::QAttribute::Float);
        posAttr->setVertexSize(3);
        PointGeometry->addAttribute(posAttr);

        auto* colorAttr = new Qt3DRender::QAttribute(PointGeometry);
        colorAttr->setName(Qt3DRender::QAttribute::defaultColorAttributeName());
        colorAttr->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
        colorAttr->setBuffer(buffer);
        colorAttr->setCount(vertexCount);
        colorAttr->setByteOffset(12);
        colorAttr->setByteStride(24);
        colorAttr->setVertexBaseType(Qt3DRender::QAttribute::Float);
        colorAttr->setVertexSize(3);
        PointGeometry->addAttribute(colorAttr);
    }
}

CFaceToQGeometry::~CFaceToQGeometry()
{
    if (!Geometry->parent())
        delete Geometry;
    if (PointGeometry && !PointGeometry->parent())
        delete PointGeometry;
}

}
