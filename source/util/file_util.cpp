/*
Copyright (c) 2017-2018 Adubbz

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "util/file_util.hpp"

#include <memory>

#include "install/simple_filesystem.hpp"
#include "nx/fs.hpp"
#include "data/byte_buffer.hpp"
#include "util/title_util.hpp"

namespace tin::util
{
    NcmContentInfo CreateNSPCNMTContentRecord(const std::string& nspPath)
    {
        // Open filesystem
        nx::fs::IFileSystem fileSystem;
        std::string nspExt = ".nsp";
          std::string nszExt = ".nsz";
        std::string rootPath = "/";
        std::string absolutePath = nspPath + "/";

        // Check if this is an nsp file
        if (nspPath.compare(nspPath.size() - nspExt.size(), nspExt.size(), nspExt) == 0 || nspPath.compare(nspPath.size() - nszExt.size(), nszExt.size(), nszExt) == 0)
        {
            fileSystem.OpenFileSystemWithId(nspPath, FsFileSystemType_ApplicationPackage, 0);
        }
        else // Otherwise assume this is an extracted NSP directory
        {
            fileSystem.OpenSdFileSystem();
            rootPath = nspPath.substr(9) + "/";
            absolutePath = nspPath + "/";
        }

        tin::install::nsp::SimpleFileSystem simpleFS(fileSystem, rootPath, absolutePath);

        // Create the path of the cnmt NCA
        auto cnmtNCAName = simpleFS.GetFileNameFromExtension("", "cnmt.nca");
        auto cnmtNCAFile = simpleFS.OpenFile(cnmtNCAName);
        u64 cnmtNCASize = cnmtNCAFile.GetSize();

        // Prepare cnmt content record
        NcmContentInfo contentRecord;
        contentRecord.content_id = tin::util::GetNcaIdFromString(cnmtNCAName);
        *(u64*)contentRecord.size = cnmtNCASize & 0xFFFFFFFFFFFF;
        contentRecord.content_type = NcmContentType_Meta;

        return contentRecord;
    }

    // NOTE: As of 7.0.0, this will only work with installed cnmt nca paths
    nx::ncm::ContentMeta GetContentMetaFromNCA(const std::string& ncaPath)
    {
        // Create the cnmt filesystem
        nx::fs::IFileSystem cnmtNCAFileSystem;
        cnmtNCAFileSystem.OpenFileSystemWithId(ncaPath, FsFileSystemType_ContentMeta, 0);
        tin::install::nsp::SimpleFileSystem cnmtNCASimpleFileSystem(cnmtNCAFileSystem, "/", ncaPath + "/");

        // Find and read the cnmt file
        auto cnmtName = cnmtNCASimpleFileSystem.GetFileNameFromExtension("", "cnmt");
        auto cnmtFile = cnmtNCASimpleFileSystem.OpenFile(cnmtName);
        u64 cnmtSize = cnmtFile.GetSize();

        tin::data::ByteBuffer cnmtBuf;
        cnmtBuf.Resize(cnmtSize);
        cnmtFile.Read(0x0, cnmtBuf.GetData(), cnmtSize);

        return nx::ncm::ContentMeta(cnmtBuf.GetData(), cnmtBuf.GetSize());
    }

    std::vector<std::string> GetNSPList()
    {
        std::vector<std::string> nspList;
        nx::fs::IFileSystem fileSystem;
        fileSystem.OpenSdFileSystem();
        nx::fs::IDirectory dir = fileSystem.OpenDirectory("/tinfoil/nsp/", FsDirOpenMode_ReadFiles);

        u64 entryCount = dir.GetEntryCount();

        auto dirEntries = std::make_unique<FsDirectoryEntry[]>(entryCount);

        dir.Read(0, dirEntries.get(), entryCount);

        for (unsigned int i = 0; i < entryCount; i++)
        {
            FsDirectoryEntry dirEntry = dirEntries[i];
            std::string dirEntryName(dirEntry.name);
            std::string nspExt = ".nsp";
            std::string nszExt = ".nsz";
            std::string xciExt = ".xci";
            std::string xczExt = ".xcz";

            if (dirEntry.type != FsDirEntryType_File ||
                (
                    dirEntryName.compare(dirEntryName.size() - nspExt.size(), nspExt.size(), nspExt) != 0 &&
                    dirEntryName.compare(dirEntryName.size() - nszExt.size(), nszExt.size(), nszExt) != 0 &&
                    dirEntryName.compare(dirEntryName.size() - xciExt.size(), xciExt.size(), xciExt) != 0 &&
                    dirEntryName.compare(dirEntryName.size() - xczExt.size(), xczExt.size(), xczExt) != 0
                    )
                )
                continue;

            nspList.push_back(dirEntry.name);
        }

        return nspList;
    }
}