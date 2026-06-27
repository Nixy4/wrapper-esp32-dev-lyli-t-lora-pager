#include "storage_impl.hpp"

#include "wrapper/sd_mmc.hpp"
#include "wrapper/sd_spi.hpp"
#include "wrapper/fatfs.hpp"
#include "wrapper/spiffs.hpp"
#include "board/lilygo/t-lora-pager.hpp"

wrapper::LilyGoLoraPager& board = wrapper::LilyGoLoraPager::GetInstance();

namespace launcher
{
}  // namespace launcher