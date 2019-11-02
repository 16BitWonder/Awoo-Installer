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

#include <cstring>
#include <sstream>
#include <filesystem>

#include "install/install_nsp.hpp"
#include "nx/fs.hpp"
#include "util/file_util.hpp"
#include "util/title_util.hpp"
#include "util/error.hpp"

#include "ui/MainApplication.hpp"
#include "nspInstall.hpp"
#include "util/config.hpp"
#include "util/util.hpp"

namespace inst::ui {
    extern MainApplication *mainApp;

    void setTopInstInfoText(std::string ourText){
        mainApp->instpage->pageInfoText->SetText(ourText);
        mainApp->CallForRender();
    }

    void setInstInfoText(std::string ourText){
        mainApp->instpage->installInfoText->SetText(ourText);
        mainApp->CallForRender();
    }

    void setInstBarPerc(double ourPercent){
        mainApp->instpage->installBar->SetVisible(true);
        mainApp->instpage->installBar->SetProgress(ourPercent);
        mainApp->CallForRender();
    }

    void loadMainMenu(){
        mainApp->LoadLayout(mainApp->mainPage);
    }

    void loadInstallScreen(){
        mainApp->instpage->pageInfoText->SetText("");
        mainApp->instpage->installInfoText->SetText("");
        mainApp->instpage->installBar->SetProgress(0);
        mainApp->instpage->installBar->SetVisible(false);
        if (inst::config::gayMode) mainApp->instpage->awooImage->SetVisible(false);
        else mainApp->instpage->awooImage->SetVisible(true);
        mainApp->LoadLayout(mainApp->instpage);
        mainApp->CallForRender();
    }
}

namespace nspInstStuff {

    void installNspFromFile(std::vector<std::filesystem::path> ourNspList, int whereToInstall)
    {
        appletLockExit();
        inst::ui::loadInstallScreen();
        bool nspInstalled = true;
        FsStorageId m_destStorageId = FsStorageId_SdCard;

        if (whereToInstall) m_destStorageId = FsStorageId_NandUser;

        try
        {
            for (unsigned int i = 0; i < ourNspList.size(); i++) {
                std::string path = "@Sdcard://" + ourNspList[i].string().erase(0, 6);

                    inst::ui::setTopInstInfoText("Installing " + inst::util::shortenString(ourNspList[i].string().erase(0, 6), 64, true) + "...");

                    nx::fs::IFileSystem fileSystem;
                    fileSystem.OpenFileSystemWithId(path, FsFileSystemType_ApplicationPackage, 0);
                    tin::install::nsp::SimpleFileSystem simpleFS(fileSystem, "/", path + "/");
                    tin::install::nsp::NSPInstallTask task(simpleFS, m_destStorageId, inst::config::ignoreReqVers);

                    printf("Preparing installation\n");
                    inst::ui::setInstInfoText("Preparing installation...");
                    task.Prepare();

                    task.Begin();
            }
        }
        catch (std::exception& e)
        {
            printf("Failed to install NSP");
            printf("%s", e.what());
            fprintf(stdout, "%s", e.what());
            inst::ui::mainApp->CreateShowDialog("Failed to install NSP!", "Partially installed NSP contents can be removed from the System Settings applet.\n\n" + (std::string)e.what(), {"OK"}, true);
            nspInstalled = false;
        }

        if(nspInstalled) {
            if (ourNspList.size() > 1) {
                if(inst::ui::mainApp->CreateShowDialog("Selected NSP files installed! Delete them from SD card?", "", {"No","Yes"}, false) == 1) {
                    for (long unsigned int i = 0; i < ourNspList.size(); i++) {
                        std::filesystem::remove(ourNspList[i]);
                    }
                }
            } else {
                if(inst::ui::mainApp->CreateShowDialog(inst::util::shortenString(ourNspList[0].string().erase(0, 6), 64, true) + " installed! Delete NSP from SD card?", "", {"No","Yes"}, false) == 1) std::filesystem::remove(ourNspList[0]);
            }
        }

        printf("Done");
        appletUnlockExit();
        inst::ui::loadMainMenu();
        return;
    }
}
