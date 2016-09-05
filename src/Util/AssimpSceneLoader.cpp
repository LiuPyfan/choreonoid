#include "AssimpSceneLoader.h"
#include "SceneDrawables.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <iostream>
#include <map>

using namespace std;
using namespace cnoid;
using namespace Assimp;

namespace cnoid{

class AssimpSceneLoaderImpl
{
public:
    AssimpSceneLoaderImpl(){ };
    SgNode* load(const std::string& fileName);
    SgPosTransform* convertAinode(aiNode* ainode);
    SgShape* convertAiMesh(unsigned int);

private:
    const aiScene* scene;
    typedef map<unsigned int, SgShape*> AiMeshToSgShapeMap;
    AiMeshToSgShapeMap aiMeshToSgShapeMap;
};

}

AssimpSceneLoader::AssimpSceneLoader()
{
    impl = new AssimpSceneLoaderImpl();
}


AssimpSceneLoader::~AssimpSceneLoader()
{
    delete impl;
}


const char* AssimpSceneLoader::format() const
{
    return "assimp";
}


SgNode* AssimpSceneLoader::load(const std::string& fileName)
{
    return impl->load(fileName);
}


SgNode* AssimpSceneLoaderImpl::load(const std::string& fileName)
{
    Importer importer;

    scene = importer.ReadFile( fileName,
            aiProcess_Triangulate            |    //三角面へ変換
            aiProcess_JoinIdenticalVertices  |    //重複する頂点座標の削除
            aiProcess_GenNormals             |    //面法線を生成
            aiProcess_FindInstances          |    //重複するメッシュを探し初出のメッシュに置き換える
            aiProcess_SortByPType                 //異なる複数のプリミティブ型からなるメッシュをサブメッシュに分割
            );

    if( !scene )
    {
        cout << aiGetErrorString() << endl;
        return 0;
    }

    SgPosTransform* transform = convertAinode(scene->mRootNode);

    return transform;

}


SgPosTransform* AssimpSceneLoaderImpl::convertAinode(aiNode* ainode)
{
    aiMatrix4x4& aiT = ainode->mTransformation;
    Affine3 T;
    T.linear() << aiT[0][0], aiT[0][1], aiT[0][2],
                  aiT[1][0], aiT[1][1], aiT[1][2],
                  aiT[2][0], aiT[2][1], aiT[2][2];
    T.translation() << aiT[0][3], aiT[1][3], aiT[2][3];
    SgPosTransform* transform = new SgPosTransform(T);

    for(int i=0; i<ainode->mNumMeshes; i++){
        SgShape* shape = convertAiMesh(ainode->mMeshes[i]);
        if(shape)
            transform->addChild(shape);
    }

    for(int i=0; i<ainode->mNumChildren; i++){
        SgPosTransform* child = convertAinode(ainode->mChildren[i]);
        if(child)
            transform->addChild(child);
    }

    if(transform->numChildren() == 0){
        delete  transform;
        transform = 0;
    }

    return transform;
}


SgShape* AssimpSceneLoaderImpl::convertAiMesh(unsigned int index)
{
    AiMeshToSgShapeMap::iterator p = aiMeshToSgShapeMap.find(index);
    if(p != aiMeshToSgShapeMap.end()){
        return p->second;
    }

    SgShape* shape = new SgShape();
    aiMesh* aimesh = scene->mMeshes[index];

    if(aimesh->HasFaces()){
        SgVertexArray* vertices = new SgVertexArray;
        SgNormalArray* normal = new SgNormalArray;
        SgTexCoordArray* texCoord = 0;
        if(aimesh->HasTextureCoords(0)){
            texCoord = new SgTexCoordArray;
        }
        SgMesh* mesh = new SgMesh;

        for (unsigned int i = 0 ; i < aimesh->mNumVertices ; i++) {
            const aiVector3D& pos = aimesh->mVertices[i];
            const aiVector3D& n_ = aimesh->mNormals[i];
         //   vertices->push_back(Vector3f(pos.x, pos.y, pos.z));
         //   normal->push_back(Vector3f(n_.x, n_.y, n_.z));
            if(texCoord){
                const aiVector3D& tc_ = aimesh->mTextureCoords[0][i];
         //       texCoord->push_back(Vector3f(tc_.x, tc_.y, tc_.z));
            }
        }
        mesh->setVertices(vertices);
        mesh->setNormals(normal);
        if(texCoord)
            mesh->setTexCoords(texCoord);

        for (unsigned int i = 0 ; i < aimesh->mNumFaces ; i++) {
            const aiFace& face = aimesh->mFaces[i];
            mesh->addTriangle(face.mIndices[0], face.mIndices[1], face.mIndices[2]);
        }

        shape->setMesh(mesh);
    }

    /*
      SgMaterial* material = shape->getOrCreateMaterial();
      SgTexture* texture = shape->getOrCreateTexture();
  */

    aiMeshToSgShapeMap[index] = shape;
    return shape;
}

