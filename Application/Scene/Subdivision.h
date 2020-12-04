#include <opensubdiv/far/topologyRefinerFactory.h>
#include <opensubdiv/far/primvarRefiner.h>
#include <opensubdiv/far/topologyDescriptor.h>

using namespace OpenSubdiv;

Sdc::SchemeType subdivisionType() {
    return Sdc::SCHEME_CATMARK;
}
Sdc::Options subdivisionOptions() {
    Sdc::Options options;
    options.SetVtxBoundaryInterpolation(Sdc::Options::VTX_BOUNDARY_EDGE_ONLY);
    return options;
}

struct Vertex {

    // Minimal required interface ----------------------
    Vertex() { }

    Vertex(Vertex const & src) {
        _position[0] = src._position[0];
        _position[1] = src._position[1];
        _position[2] = src._position[2];
    }

    void Clear( void * =0 ) {
        _position[0]=_position[1]=_position[2]=0.0f;
    }

    void AddWithWeight(Vertex const & src, float weight) {
        _position[0]+=weight*src._position[0];
        _position[1]+=weight*src._position[1];
        _position[2]+=weight*src._position[2];
    }

    // Public interface ------------------------------------
    void SetPosition(float x, float y, float z) {
        _position[0]=x;
        _position[1]=y;
        _position[2]=z;
    }

    const float * GetPosition() const {
        return _position;
    }

private:
    float _position[3];
};


// commented for future switch
/*
namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

namespace Far {

template <>
bool
TopologyRefinerFactory<CMeshImpl>::resizeComponentTopology(
    TopologyRefiner & refiner, CMeshImpl const & conv) {

    // Faces and face-verts
    int nfaces = conv.n_faces();
    setNumBaseFaces(refiner, nfaces);
    for (auto face : conv.faces()) {
        int i = 0;
        for (auto vertex : face.vertices())
            i++;
        setNumBaseFaceVertices(refiner, face.idx(), i);
    }


    // Edges and edge-faces
    int nedges = conv.n_edges();
    setNumBaseEdges(refiner, nedges);
    for (auto edge : conv.edges()) {
        setNumBaseEdgeFaces(refiner, edge.idx(), 2);
    }

    // Vertices and vert-faces and vert-edges
    setNumBaseVertices(refiner, conv.n_vertices());
    for (auto vertex : conv.vertices())
    {
        int i = 0;
        for (auto edge : vertex.edges())
            i++;
        setNumBaseVertexEdges(refiner, vertex.idx(), i);
        i = 0;
        for (auto face : vertex.faces())
            i++;
        setNumBaseVertexFaces(refiner, vertex.idx(), i);
    }
    return true;
}

template <>
bool
TopologyRefinerFactory<CMeshImpl>::assignComponentTopology(
    TopologyRefiner & refiner, CMeshImpl const & conv) {

    { // Face relations:
        for (auto face : conv.faces())
        {
            Far::IndexArray dstFaceVerts = getBaseFaceVertices(refiner, face.idx());
            Far::IndexArray dstFaceEdges = getBaseFaceEdges(refiner, face.idx());
            int i = 0;
            for (auto vert : face.vertices()) {
                dstFaceVerts[i] = vert.idx();
                i++;
            }
            i = 0;
            for (auto edge : face.edges())
            {
                dstFaceEdges[i] = edge.idx();
                i++;
            }
        }

    }

    {

        for (auto edge : conv.edges()) {
            //  Edge-vertices:
            Far::IndexArray dstEdgeVerts = getBaseEdgeVertices(refiner, edge.idx());
            dstEdgeVerts[0] = edge.v1().idx();
            dstEdgeVerts[1] = edge.v0().idx();

            //  Edge-faces
            Far::IndexArray dstEdgeFaces = getBaseEdgeFaces(refiner, edge.idx());
            dstEdgeFaces[0] = edge.h1().face().idx();
            dstEdgeFaces[1] = edge.h0().face().idx();

        }
    }


    { // Vertex relations

        for (auto v_itr = conv.vertices_begin(); v_itr != conv.vertices_end() ; ++v_itr)
        {
            Far::IndexArray vertFaces = getBaseVertexFaces(refiner, v_itr->idx());

            int i = 0;
            for (auto face : v_itr->faces()) {
                vertFaces[i] = face.idx();
                i++;
            }
            //  Vert-Edges:
            Far::IndexArray vertEdges = getBaseVertexEdges(refiner, v_itr->idx());
            i = 0;
            for (auto edge : v_itr->edges()) {
                vertEdges[i] = edge.idx();
            }
        }

    }

    populateBaseLocalIndices(refiner);

    return true;
};


template <>
bool
TopologyRefinerFactory<CMeshImpl>::assignComponentTags(
    TopologyRefiner & refiner, CMeshImpl const & conv) {

    // arbitrarily sharpen the 4 bottom edges of the pyramid to 2.5f
    for (int vertex=0; vertex < conv.n_vertices(); ++vertex) {
        ///setBaseVertexSharpness(refiner, vertex, conv.data(conv.vertex_handle(vertex)).sharpness());

    }
    return true;
}
} // namespace Far

} // namespace OPENSUBDIV_VERSION
} // namespace OpenSubdiv
 */