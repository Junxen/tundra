// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_AssetModule_h
#define incl_AssetModule_h

#include "Foundation.h"
#include "ModuleInterface.h"

#include "ConsoleCommandServiceInterface.h"
#include "AssetProviderInterface.h"
#include "AssetModuleApi.h"

namespace Foundation
{
    class Framework;
}

namespace ProtocolUtilities
{
	class ProtocolModuleInterface;
}

namespace Asset
{
    class AssetManager;
    typedef boost::shared_ptr<AssetManager> AssetManagerPtr;
    
    /** \defgroup AssetModuleClient AssetModule Client Interface
        This page lists the public interface of the AssetModule,
        which consists of implementing Foundation::AssetServiceInterface and
        Foundation::AssetInterface

        For details on how to use the public interface, see \ref AssetModule "Using the asset module".
    */

    //! Asset module.
    class ASSET_MODULE_API AssetModule : public Foundation::ModuleInterfaceImpl
    {
    public:
        AssetModule();
        virtual ~AssetModule();

        virtual void Load();
        virtual void Unload();
        virtual void Initialize();
        virtual void Uninitialize();
        virtual void PostInitialize();
        virtual void Update(Core::f64 frametime);

        virtual bool HandleEvent(
            Core::event_category_id_t category_id,
            Core::event_id_t event_id, 
            Foundation::EventDataInterface* data);

		virtual void SubscribeToNetworkEvents(boost::weak_ptr<ProtocolUtilities::ProtocolModuleInterface> currentProtocolModule);
		void UnsubscribeNetworkEvents();

        MODULE_LOGGING_FUNCTIONS

        //! callback for console command
        Console::CommandResult ConsoleRequestAsset(const Core::StringVector &params);
        
        //! returns name of this module. Needed for logging.
        static const std::string &NameStatic() { return Foundation::Module::NameFromType(type_static_); }

        static const Foundation::Module::Type type_static_ = Foundation::Module::MT_Asset;

    private:
        //! UDP asset provider
        Foundation::AssetProviderPtr udp_asset_provider_;

        //! XMLRPC asset provider
        Foundation::AssetProviderPtr xmlrpc_asset_provider_;

        //! Http asset provider
		Foundation::AssetProviderPtr http_asset_provider_;
        
        //! asset manager
        AssetManagerPtr manager_;
        
        //! category id for incoming messages
        Core::event_category_id_t inboundcategory_id_;

        //! framework id for internal events
        Core::event_category_id_t framework_category_id_;

		//! Pointer to current ProtocolModule
		boost::weak_ptr<ProtocolUtilities::ProtocolModuleInterface> protocolModule_;
    };
}

#endif
