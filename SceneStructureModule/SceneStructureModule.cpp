/**
 *  For conditions of distribution and use, see copyright notice in license.txt
 *
 *  @file   SceneStructureModule.cpp
 *  @brief  Provides Scene Structure and Assets windows and raycast drag-and-drop import of
 *          .mesh, .scene, .xml and .nbf files to the main window.
 */

#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#include "SceneStructureModule.h"
#include "SceneStructureWindow.h"
#include "AssetsWindow.h"
#include "SupportedFileTypes.h"
#include "AddContentWindow.h"

#include "Console.h"
#include "UiServiceInterface.h"
#include "Input.h"
#include "RenderServiceInterface.h"
#include "SceneImporter.h"
#include "EC_OgreCamera.h"
#include "EC_Placeable.h"
#include "NaaliUi.h"
#include "NaaliGraphicsView.h"
#include "NaaliMainWindow.h"
#include "LoggingFunctions.h"
#include "SceneDesc.h"

DEFINE_POCO_LOGGING_FUNCTIONS("SceneStructure");

#ifdef ASSIMP_ENABLED
#include <OpenAssetImport.h>
#endif

//#include <OgreCamera.h>

#include "MemoryLeakCheck.h"

SceneStructureModule::SceneStructureModule() :
    IModule("SceneStructure"),
    sceneWindow(0),
    assetsWindow(0)
{
}

SceneStructureModule::~SceneStructureModule()
{
    SAFE_DELETE(sceneWindow);
}

void SceneStructureModule::PostInitialize()
{
    framework_->Console()->RegisterCommand("scenestruct", "Shows the Scene Structure window.", this, SLOT(ShowSceneStructureWindow()));
    framework_->Console()->RegisterCommand("assets", "Shows the Assets window.", this, SLOT(ShowAssetsWindow()));

    inputContext = framework_->GetInput()->RegisterInputContext("SceneStructureInput", 90);
    connect(inputContext.get(), SIGNAL(KeyPressed(KeyEvent *)), this, SLOT(HandleKeyPressed(KeyEvent *)));

    connect(framework_->Ui()->GraphicsView(), SIGNAL(DragEnterEvent(QDragEnterEvent *)), SLOT(HandleDragEnterEvent(QDragEnterEvent *)));
    connect(framework_->Ui()->GraphicsView(), SIGNAL(DragMoveEvent(QDragMoveEvent *)), SLOT(HandleDragMoveEvent(QDragMoveEvent *)));
    connect(framework_->Ui()->GraphicsView(), SIGNAL(DropEvent(QDropEvent *)), SLOT(HandleDropEvent(QDropEvent *)));
}

QList<Scene::Entity *> SceneStructureModule::InstantiateContent(const QString &filename, Vector3df worldPos, bool clearScene)
{
    return InstantiateContent(filename, worldPos, SceneDesc(), clearScene);
}

QList<Scene::Entity *> SceneStructureModule::InstantiateContent(const QString &filename, Vector3df worldPos, const SceneDesc &desc, bool clearScene)
{
    return InstantiateContent(QStringList(QStringList() << filename), worldPos, desc, clearScene);
}

QList<Scene::Entity *> SceneStructureModule::InstantiateContent(const QStringList &filenames, Vector3df worldPos, const SceneDesc &desc, bool clearScene)
{
    QList<Scene::Entity *> ret;

    const Scene::ScenePtr &scene = framework_->GetDefaultWorldScene();
    if (!scene)
    {
        LogError("Could not retrieve default world scene.");
        return ret;
    }

    QList<SceneDesc> sceneDescs;

    foreach(QString filename, filenames)
    {
        if (!IsSupportedFileType(filename))
        {
            LogError("Unsupported file extension: " + filename.toStdString());
            continue;
        }

        if (filename.endsWith(cOgreSceneFileExtension, Qt::CaseInsensitive))
        {
            //boost::filesystem::path path(filename.toStdString());
            //std::string dirname = path.branch_path().string();

            TundraLogic::SceneImporter importer(scene);
//            sceneDesc = importer.GetSceneDescForScene(filename);
            sceneDescs.append(importer.GetSceneDescForScene(filename));
/*
            ret = importer.Import(filename.toStdString(), dirname, "./data/assets",
                Transform(worldPos, Vector3df(), Vector3df(1,1,1)), AttributeChange::Default, clearScene, true, false);
            if (ret.empty())
                LogError("Import failed");
            else
                LogInfo("Import successful. " + ToString(ret.size()) + " entities created.");
*/
        }
        else if (filename.endsWith(cOgreMeshFileExtension, Qt::CaseInsensitive))
        {
//            boost::filesystem::path path(filename.toStdString());
//            std::string dirname = path.branch_path().string();

            TundraLogic::SceneImporter importer(scene);
            sceneDescs.append(importer.GetSceneDescForMesh(filename));
//            sceneDesc = importer.GetSceneDescForMesh(filename);
/*
            Scene::EntityPtr entity = importer.ImportMesh(filename.toStdString(), dirname, "./data/assets",
                Transform(worldPos, Vector3df(), Vector3df(1,1,1)), std::string(), AttributeChange::Default, true);
            if (entity)
                ret << entity.get();

            return ret;
*/
        }
        else if (filename.toLower().indexOf(cTundraXmlFileExtension) != -1 && filename.toLower().indexOf(cOgreMeshFileExtension) == -1)
        {
//        ret = scene->LoadSceneXML(filename.toStdString(), clearScene, false, AttributeChange::Replicate);
//            sceneDesc = scene->GetSceneDescFromXml(filename);
            sceneDescs.append(scene->GetSceneDescFromXml(filename));
        }
        else if (filename.toLower().indexOf(cTundraBinFileExtension) != -1)
        {
//        ret = scene->CreateContentFromBinary(filename, true, AttributeChange::Replicate);
//            sceneDesc = scene->GetSceneDescFromBinary(filename);
            sceneDescs.append(scene->GetSceneDescFromXml(filename));
        }
        else
        {
#ifdef ASSIMP_ENABLED
            boost::filesystem::path path(filename.toStdString());
            AssImp::OpenAssetImport assimporter;
            QString extension = QString(path.extension().c_str()).toLower();
            if (assimporter.IsSupportedExtension(extension))
            {
                std::string dirname = path.branch_path().string();
                std::vector<AssImp::MeshData> meshNames;
                assimporter.GetMeshData(filename, meshNames);

                TundraLogic::SceneImporter sceneimporter(scene);
                for (size_t i=0 ; i<meshNames.size() ; ++i)
                {
                    Scene::EntityPtr entity = sceneimporter.ImportMesh(meshNames[i].file_.toStdString(), dirname, meshNames[i].transform_,
                        std::string(), "local://", AttributeChange::Default, false, meshNames[i].name_.toStdString());
                    if (entity)
                        ret.append(entity.get());
                }

                return ret;
            }
#endif
        }
    }

    if (!sceneDescs.isEmpty())
    {
        AddContentWindow *addContent = new AddContentWindow(framework_, scene);
//        addContent->AddDescription(sceneDesc);
        addContent->AddDescription(sceneDescs[0]);
        if (worldPos != Vector3df())
            addContent->AddPosition(worldPos);
        addContent->move((framework_->Ui()->MainWindow()->pos()) + QPoint(400, 200));
        addContent->show();
    }

    return ret;
}

void SceneStructureModule::CentralizeEntitiesTo(const Vector3df &pos, const QList<Scene::Entity *> &entities)
{
    Vector3df minPos(1e9f, 1e9f, 1e9f);
    Vector3df maxPos(-1e9f, -1e9f, -1e9f);

    foreach(Scene::Entity *e, entities)
    {
        EC_Placeable *p = e->GetComponent<EC_Placeable>().get();
        if (p)
        {
            Vector3df pos = p->transform.Get().position;
            minPos.x = std::min(minPos.x, pos.x);
            minPos.y = std::min(minPos.y, pos.y);
            minPos.z = std::min(minPos.z, pos.z);
            maxPos.x = std::max(maxPos.x, pos.x);
            maxPos.y = std::max(maxPos.y, pos.y);
            maxPos.z = std::max(maxPos.z, pos.z);
        }
    }

    // We assume that world's up axis is Z-coordinate axis.
    Vector3df importPivotPos = Vector3df((minPos.x + maxPos.x) / 2, (minPos.y + maxPos.y) / 2, minPos.z);
    Vector3df offset = pos - importPivotPos;

    foreach(Scene::Entity *e, entities)
    {
        EC_Placeable *p = e->GetComponent<EC_Placeable>().get();
        if (p)
        {
            Transform t = p->transform.Get();
            t.position += offset;
            p->transform.Set(t, AttributeChange::Default);
        }
    }
}

bool SceneStructureModule::IsSupportedFileType(const QString &filename)
{
    if (filename.endsWith(cTundraXmlFileExtension, Qt::CaseInsensitive) ||
        filename.endsWith(cTundraBinFileExtension, Qt::CaseInsensitive) ||
        filename.endsWith(cOgreMeshFileExtension, Qt::CaseInsensitive) ||
        filename.endsWith(cOgreSceneFileExtension, Qt::CaseInsensitive))
    {
        return true;
    }
    else
    {
#ifdef ASSIMP_ENABLED
        boost::filesystem::path path(filename.toStdString());
        AssImp::OpenAssetImport assimporter;
        QString extension = QString(path.extension().c_str()).toLower();
        if (assimporter.IsSupportedExtension(extension))
            return true;
#endif
        return false;
    }
}

void SceneStructureModule::ShowSceneStructureWindow()
{
    /*UiServiceInterface *ui = framework_->GetService<UiServiceInterface>();
    if (!ui)
        return;*/

    if (sceneWindow)
    {
        //ui->ShowWidget(sceneWindow);
        sceneWindow->show();
        return;
    }

    NaaliUi *ui = GetFramework()->Ui();
    if (!ui)
        return;

    sceneWindow = new SceneStructureWindow(framework_);
    //sceneWindow->move(200,200);
    sceneWindow->SetScene(framework_->GetDefaultWorldScene());
    sceneWindow->setParent(ui->MainWindow());
    sceneWindow->setWindowFlags(Qt::Tool);
    sceneWindow->show();

    //ui->AddWidgetToScene(sceneWindow);
    //ui->ShowWidget(sceneWindow);
}

void SceneStructureModule::ShowAssetsWindow()
{
    /*UiServiceInterface *ui = framework_->GetService<UiServiceInterface>();
    if (!ui)
        return;*/

    if (assetsWindow)
    {
        //ui->ShowWidget(assetsWindow);
        assetsWindow->show();
        return;
    }

    NaaliUi *ui = GetFramework()->Ui();
    if (!ui)
        return;

    assetsWindow = new AssetsWindow(framework_);
    assetsWindow->setParent(ui->MainWindow());
    assetsWindow->setWindowFlags(Qt::Tool);
    assetsWindow->show();

    //ui->AddWidgetToScene(assetsWindow);
    //ui->ShowWidget(assetsWindow);
}

void SceneStructureModule::HandleKeyPressed(KeyEvent *e)
{
    if (e->eventType != KeyEvent::KeyPressed || e->keyPressCount > 1)
        return;

    Input &input = *framework_->GetInput();

    const QKeySequence &showSceneStruct = input.KeyBinding("ShowSceneStructureWindow", QKeySequence(Qt::ShiftModifier + Qt::Key_S));
    const QKeySequence &showAssets = input.KeyBinding("ShowAssetsWindow", QKeySequence(Qt::ShiftModifier + Qt::Key_A));

    QKeySequence keySeq(e->keyCode | e->modifiers);
    if (keySeq == showSceneStruct)
        ShowSceneStructureWindow();
    if (keySeq == showAssets)
        ShowAssetsWindow();
}

void SceneStructureModule::HandleDragEnterEvent(QDragEnterEvent *e)
{
    // If at least one file is supported, accept.
    if (e->mimeData()->hasUrls())
        foreach(QUrl url, e->mimeData()->urls())
            if (IsSupportedFileType(url.path()))
                e->accept();
}

void SceneStructureModule::HandleDragMoveEvent(QDragMoveEvent *e)
{
    // If at least one file is supported, accept.
    if (e->mimeData()->hasUrls())
        foreach(QUrl url, e->mimeData()->urls())
            if (IsSupportedFileType(url.path()))
                e->accept();
}

void SceneStructureModule::HandleDropEvent(QDropEvent *e)
{
    if (e->mimeData()->hasUrls())
    {
        QList<Scene::Entity *> importedEntities;

        Foundation::RenderServiceInterface *renderer = framework_->GetService<Foundation::RenderServiceInterface>();
        if (!renderer)
            return;

        Vector3df worldPos;
        RaycastResult* res = renderer->Raycast(e->pos().x(), e->pos().y());
        if (!res->entity_)
        {
            // No entity hit, use camera's position with hard-coded offset.
            const Scene::ScenePtr &scene = framework_->GetDefaultWorldScene();
            if (!scene)
                return;

            foreach(Scene::EntityPtr cam, scene->GetEntitiesWithComponent(EC_OgreCamera::TypeNameStatic()))
                if (cam->GetComponent<EC_OgreCamera>()->IsActive())
                {
                    EC_Placeable *placeable = cam->GetComponent<EC_Placeable>().get();
                    if (placeable)
                    {
                        //Ogre::Ray ray = cam->GetComponent<EC_OgreCamera>()->GetCamera()->getCameraToViewportRay(e->pos().x(), e->pos().y());
                        Quaternion q = placeable->GetOrientation();
                        Vector3df v = q * -Vector3df::UNIT_Z;
                        //Ogre::Vector3 oV = ray.getPoint(20);
                        worldPos = /*Vector3df(oV.x, oV.y, oV.z);*/ placeable->GetPosition() + v * 20;
                        break;
                    }
                }
        }
        else
            worldPos = res->pos_;

        foreach (QUrl url, e->mimeData()->urls())
        {
            QString filename = url.path();
#ifdef _WINDOWS
            // We have '/' as the first char on windows and the filename
            // is not identified as a file properly. But on other platforms the '/' is valid/required.
            filename = filename.mid(1);
#endif
            importedEntities.append(InstantiateContent(filename, worldPos/*Vector3df()*/, false));
        }

        // Calculate import pivot and offset for new content
        //if (importedEntities.size())
        //    CentralizeEntitiesTo(worldPos, importedEntities);

        e->acceptProposedAction();
    }
}

extern "C" void POCO_LIBRARY_API SetProfiler(Foundation::Profiler *profiler);
void SetProfiler(Foundation::Profiler *profiler)
{
    Foundation::ProfilerSection::SetProfiler(profiler);
}

POCO_BEGIN_MANIFEST(IModule)
   POCO_EXPORT_CLASS(SceneStructureModule)
POCO_END_MANIFEST
