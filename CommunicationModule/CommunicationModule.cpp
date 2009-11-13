#include <StableHeaders.h>
#include "CommunicationModule.h"
#include "OpensimIM/ConnectionProvider.h"
#include "TelepathyIM/ConnectionProvider.h"

namespace Communication
{

	CommunicationModule::CommunicationModule(void) 
        : ModuleInterfaceImpl("CommunicationModule"), qt_ui_(0), opensim_ui_(0), communication_service_(0), test_(0), event_category_networkstate_(0), event_category_framework_(0)
	{
	}

	CommunicationModule::~CommunicationModule(void)
	{
	}

	void CommunicationModule::Load(){}
	void CommunicationModule::Unload(){}

	void CommunicationModule::Initialize() 
	{
        event_category_framework_ = framework_->GetEventManager()->QueryEventCategory("Framework");
		LogInfo("Initialized.");
	}

	void CommunicationModule::PostInitialize()
	{
		Foundation::EventManagerPtr event_manager = framework_->GetEventManager(); 

		if ( communication_service_ == NULL)
		{
			return;
		}

		QStringList protocols = communication_service_->GetSupportedProtocols();
		if (protocols.size() == 0)
			LogInfo("No IM protocols supported");
		else
		{
			for (QStringList::iterator i = protocols.begin(); i != protocols.end(); ++i)
			{
				QString message = "IM protocol support for: ";
				message.append(*i);
				LogInfo( message.toStdString() );
			}
		}
	}

    void CommunicationModule::RuntimeInitialize()
    {
        event_category_networkstate_ = framework_->GetEventManager()->QueryEventCategory("NetworkState");

        if (communication_service_ == 0)
        {
            CommunicationService::CreateInstance(framework_);
		    communication_service_ = CommunicationService::GetInstance();

            OpensimIM::ConnectionProvider* opensim = new OpensimIM::ConnectionProvider(framework_);
		    communication_service_->RegisterConnectionProvider(opensim);

		    TelepathyIM::ConnectionProvider* telepathy = new TelepathyIM::ConnectionProvider(framework_);
		    communication_service_->RegisterConnectionProvider(telepathy);
        }

        if (!qt_ui_)
            qt_ui_ = new CommunicationUI::QtGUI(framework_);
        if (!test_)
		    test_ = new CommunicationTest::Test(framework_);

        PostInitialize();
    }

	void CommunicationModule::Uninitialize()
	{
		if (qt_ui_)
			SAFE_DELETE(qt_ui_);

		if (opensim_ui_)
			SAFE_DELETE(opensim_ui_);

		if (communication_service_)
			SAFE_DELETE(communication_service_);

        if (test_)
            SAFE_DELETE(test_);

		LogInfo("Uninitialized.");
	}

	void CommunicationModule::Update(Core::f64 frametime)
	{
	}

    bool CommunicationModule::HandleEvent(Core::event_category_id_t category_id, Core::event_id_t event_id, Foundation::EventDataInterface* data)
    {
		if ( category_id == event_category_networkstate_  )
		{
			if ( event_id == ProtocolUtilities::Events::EVENT_SERVER_CONNECTED )
				if (opensim_ui_ == 0)
				{
                    
                    boost::weak_ptr<ProtocolUtilities::ProtocolModuleInterface> currentProtocolModule = framework_->GetModuleManager()->GetModule<RexLogic::RexLogicModule>(Foundation::Module::MT_WorldLogic).lock().get()->GetServerConnection()->GetCurrentProtocolModuleWeakPointer();
					if (currentProtocolModule.lock().get())
                    {
                        ProtocolUtilities::ClientParameters clientParams = currentProtocolModule.lock()->GetClientParameters();
                        opensim_ui_ = new CommunicationUI::OpenSimChat(framework_, clientParams);
                    }
				}
			if ( event_id == ProtocolUtilities::Events::EVENT_SERVER_DISCONNECTED )
				if (opensim_ui_ != 0)
					SAFE_DELETE(opensim_ui_);
		}
        else if ( category_id == event_category_framework_ && event_id == Foundation::NETWORKING_REGISTERED )
        {
            RuntimeInitialize();
        }

		if (communication_service_)
			return dynamic_cast<CommunicationService*>( communication_service_ )->HandleEvent(category_id, event_id, data);

		return false;
    }    

}

extern "C" void POCO_LIBRARY_API SetProfiler(Foundation::Profiler *profiler);
void SetProfiler(Foundation::Profiler *profiler)
{
    Foundation::ProfilerSection::SetProfiler(profiler);
}

using namespace Communication;

POCO_BEGIN_MANIFEST(Foundation::ModuleInterface)
POCO_EXPORT_CLASS(CommunicationModule)
POCO_END_MANIFEST
