#include "StableHeaders.h"
#include "AssetDefines.h"
#include "AssetTransfer.h"
#include "AssetModule.h"

using namespace RexTypes;

namespace Asset
{
    AssetTransfer::AssetTransfer() :
        size_(0),
        received_(0),
        time_(0.0)
    {
    }
    
    AssetTransfer::~AssetTransfer()
    {
    }
    
    bool AssetTransfer::Ready() const
    {
        if (!size_) 
            return false; // No header received, size not known yet
        
        return received_ >= size_;
    }
    
    Core::uint AssetTransfer::GetReceivedContinuous() const
    {
        Core::uint size = 0;
        
        DataPacketMap::const_iterator i = data_packets_.begin();
        
        Core::uint expected_index = 0;
      
        while (i != data_packets_.end())
        {
            if (i->first != expected_index)
                break;
            
            size += i->second.size();
            
            ++expected_index;
            ++i;
        }
        
        return size;
    }
    
    void AssetTransfer::ReceiveData(Core::uint packet_index, const Core::u8* data, Core::uint size)
    {
        time_ = 0.0;
        
        if (!size)
        {
            AssetModule::LogWarning("Trying to store zero bytes of data");
            return;
        }
        
        if (!data_packets_[packet_index].size())
        {
            data_packets_[packet_index].resize(size);
            memcpy(&data_packets_[packet_index][0], data, size);
            received_ += size;
        }
        else
        {
            AssetModule::LogWarning("Already received asset data packet index " + Core::ToString<Core::uint>(packet_index));
        }
    }
    
    void AssetTransfer::AssembleData(Core::u8* buffer) const
    {
        DataPacketMap::const_iterator i = data_packets_.begin();
        
        Core::uint expected_index = 0;
      
        while (i != data_packets_.end())
        {
            if (i->first != expected_index)
                break;
            
            memcpy(buffer, &i->second[0], i->second.size());
            buffer += i->second.size();
            
            ++expected_index;
            ++i;
        }
    }
}