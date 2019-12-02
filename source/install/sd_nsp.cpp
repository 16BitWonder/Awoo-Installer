#include "install/sd_nsp.hpp"
#include "install/pfs0.hpp"
#include "error.hpp"
#include "debug.h"
#include "nx/nca_writer.h"

namespace tin::install::nsp
{
    SdNSP::SdNSP(std::string path)
    {
        m_nspFile = fopen((path).c_str(), "rb");
        if (!m_nspFile)
            THROW_FORMAT("can't open file at %s\n", path.c_str());
    }

    SdNSP::~SdNSP()
    {
        fclose(m_nspFile);
    }

    void SdNSP::StreamToPlaceholder(std::shared_ptr<nx::ncm::ContentStorage>& contentStorage, NcmContentId ncaId)
    {
        const PFS0FileEntry* fileEntry = this->GetFileEntryByNcaId(ncaId);
        std::string ncaFileName = this->GetFileEntryName(fileEntry);

        LOG_DEBUG("Retrieving %s\n", ncaFileName.c_str());
        size_t ncaSize = fileEntry->fileSize;

        NcaWriter writer(ncaId, contentStorage);

        float progress;

        u64 fileStart = GetDataOffset() + fileEntry->dataOffset;
        u64 fileOff = 0;
        size_t readSize = 0x400000; // 4MB buff
        auto readBuffer = std::make_unique<u8[]>(readSize);

        try
        {
            //inst::ui::setInstInfoText("Installing " + ncaFileName + "...");
            //inst::ui::setInstBarPerc(0);
            while (fileOff < ncaSize)
            {
                progress = (float) fileOff / (float) ncaSize;

                if (fileOff % (0x400000 * 3) == 0) {
                    LOG_DEBUG("> Progress: %lu/%lu MB (%d%s)\r", (fileOff / 1000000), (ncaSize / 1000000), (int)(progress * 100.0), "%");
                    //inst::ui::setInstBarPerc((double)(progress * 100.0));
                }

                if (fileOff + readSize >= ncaSize) readSize = ncaSize - fileOff;

                this->BufferData(readBuffer.get(), fileOff + fileStart, readSize);
                writer.write(readBuffer.get(), readSize);

                fileOff += readSize;
            }
            //inst::ui::setInstBarPerc(100);
        }
        catch (std::exception& e)
        {
            LOG_DEBUG("something went wrong: %s\n", e.what());
        }

        writer.close();
    }

    void SdNSP::BufferData(void* buf, off_t offset, size_t size)
    {
        fseeko(m_nspFile, offset, SEEK_SET);
        fread(buf, 1, size, m_nspFile);
    }
}
