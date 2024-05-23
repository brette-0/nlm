/*

    nlm (NES Library Manager)

    - filter URL to whitelist filetype *.zip


*/


#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <functional>
#include <vector>
#include <format>
#include <cstdlib>

#include <git2/global.h>
#include <git2/clone.h>

// debug

#include <git2/errors.h>
#include <git2/common.h>

const std::string nlm              = std::string("c:/nlm");     // MIGRATE TO SYSTEM ENVIRONMENT VARIABLE (install process change?)

namespace fs = std::filesystem;
using namespace nlohmann;

CURL *curl;
CURLcode curl_code = CURLE_OK;

char CURL_ERROR_BUFFER[0];

void rw(const std::string& tar);
inline void mv(std::pair<std::string, std::string> task);

namespace net {
    size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    size_t WriteCallback_nofstream(void* contents, size_t size, size_t nmemb, void* userp);
    bool download(std::string target_url, std::string target_path);
    std::vector<byte> download(const std::string& url);
}

namespace commands {
    void list();
    void remove(std::string* listing);
    void connect(std::pair<std::string, std::string>* cdata);
    void install(std::string* library);
    std::function<void(void*)> commands[] = {
        [](void* _) -> void {list();},
        [](void* _) -> void {remove((std::string*)_);},
        [](void* _) -> void {connect((std::pair<std::string, std::string>*)_);},
        [](void* _) -> void {install((std::string*)_);}};
}

int main(int argc, char* args[]){    
    // check for files existing
    if (fs::exists(nlm + "/temp/")){
        rw(nlm + "/temp");
        fs::remove_all(nlm + "/temp");
    }
    if (!fs::exists(nlm)){
    
        std::cout << "required nlm directory missing, would you like to create one? [default = Y] (Y/N) : ";
        char response[0]; std::cin.getline(response, 2);

        switch (response[0]){
            default:
                std::cout << "operation aborted, exiting...\n";
                return 1;
            case 'Y':
            case 'y':
            case '\n': break;
        }

        if (!fs::create_directory(nlm)){
            std::cerr << "Error : Could not create required nlm directory, exiting...\n";
            return 1;
        }

        std::ofstream listings = std::ofstream(nlm + "/listings.json");
        listings << R"(
{ "default" : "https://raw.githubusercontent.com/brette-0/nlm/main/listing.default.json"
})";
        std::ofstream installed = std::ofstream(nlm + "/installed.json");
        fs::create_directory(nlm + "/libs");
        fs::create_directory(nlm + "/configs");
        installed << R"({})";
        installed.close();
    }

    if ((argc - 2) < 0){
        std::cout << "No tasks to complete, exiting...\n";
        return 0;
    }

    std::hash<std::string> hash_string;

    byte command_id;
    void* argptr;
    std::string argstr;
    std::pair<std::string, std::string> argpairstr;

    switch ((uint64_t)hash_string(args[1])){
        case 14384129217966325695u:
        case 3719935594332938432u:
            command_id = 0;
            break;

        case 7629267266967465055u:
        case 207937403940463093u:
            command_id = 1;
            if (argc == 2){
                std::cout << "No arguement, cannot remove\n";
                return 1;
            }
            argstr = std::string(args[2]);
            argptr = &argstr;
            break;
        
        case 1753817111081909669u:
        case 9196625560755686471u:
            command_id = 2;
            if (argc < 4){
                std::cout << "Too few arguements, cannot connect.\n";
                return 1;
            }

            argpairstr = {args[2], args[3]};
            argptr = &argpairstr;
            break;

        case 4515630727894863556u:
        case 6849809912359275844u:
            command_id = 3;
            argstr = std::string(args[2]);
            if (!argstr.contains('/')){
                std::cerr << "listing must be specified!\n";
                return 1;
            }
            argptr = &argstr;
            break;

        default:
            std::cout << R"(couldn't recognize that command, please use : 'nlm -h' or 'nlm --help' to see commands
)";
        return 1;
    }
    
    
    commands::commands[command_id](argptr);
}




namespace net {
    size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        std::ofstream* stream = static_cast<std::ofstream*>(userp);
        size_t written = stream->write(static_cast<char*>(contents), size * nmemb).tellp();
        return written;
    }

    bool download(std::string target_url, std::string target_path){
        std::ofstream target = std::ofstream(target_path, std::ios::binary);
        curl = curl_easy_init();
        if (!curl){
            // bla bla bla error
            return 1;
        }

        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, CURL_ERROR_BUFFER);


        curl_easy_setopt(curl, CURLOPT_URL, target_url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, net::WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &target);

        curl_code = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (curl_code){
            target << R"({})";
            target.close();
            return 1;
        }

        target.close();
        return 0;
    }

    std::vector<byte> download(const std::string& url) {
        std::vector<byte> buffer;
        curl = curl_easy_init();
        if (!curl){
            // bla bla bla error
            return {};
        }

        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, CURL_ERROR_BUFFER);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, net::WriteCallback_nofstream);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
        curl_code = curl_easy_perform(curl);
        if(curl_code != CURLE_OK) {
            curl_easy_cleanup(curl);
            return {};
        }
        curl_easy_cleanup(curl);
        return buffer;
    }

    size_t WriteCallback_nofstream(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t totalSize = size * nmemb;
        std::vector<byte>* buffer = (std::vector<byte>*)userp;
        buffer->insert(buffer->end(), (byte*)contents, (byte*)contents + totalSize);
        return totalSize;
    }
}



namespace commands {
    void list(){
        json listings = json::parse(std::ifstream(nlm + "/listings.json")); 
        std::cout << "Found " << listings.size() << " listings.\n";
        json listing;
        for (json::iterator iter = listings.begin(); iter != listings.end(); iter++){
            std::vector<byte> response = net::download(iter.value());
            if (response.empty()){
                std::cerr << "unable to access " << iter.key() << " : "<< CURL_ERROR_BUFFER << '\n';
                continue;
            } else std::cout << R"(listing : ")" << iter.key() << R"(" : )" << iter.value() << '\n';

            listing = json::parse(std::string(response.begin(), response.end()));
            
            #define str(_string) library[_string].get<std::string>()
            for (const auto& library : listing){
                std::cout << std::format<std::string>("    Name : {}\n    Author : {}\n    Assembler : {}\n    Version : {}\n    Host : {}\n\n",
                 str("name"), str("author"), str("version"), str("assembler"), str("url")); 
            }
        }
    }
    
    void remove(std::string* listing){
        json listings = json::parse(std::ifstream(nlm + "/listings.json"));
        
        listings.erase(*listing);
        std::ofstream _listings = std::ofstream(nlm + "/listings.json"); _listings << listings.dump(4);
    }

    void connect(std::pair<std::string, std::string>* cdata){
        json listings = json::parse(std::ifstream(nlm + "/listings.json"));
        if (listings.size())
            if (listings.find(cdata->first) == listings.end()){ // this does NOT mean that its already connected
                std::cout << "error, listing already connected\n";
                return;
            }

        listings[cdata->first] = cdata->second;
        std::ofstream buffer(nlm + "/listings.json");
        buffer << listings.dump(4);
        buffer.close();
    }

    void install(std::string* library){
        std::string _listing = library->substr(0, library->find('/'));
        json listing;
        *library = library->substr(library->find('/')+1);

        json listings = json::parse(std::ifstream(nlm + "/listings.json"));
        if (listings.empty()){
            // return error message;
            return;
        }

        try {
            // target = listings[listing]
            listing = json::parse(net::download(listings[_listing]));
        } catch (const json::exception& e){
            // return 'index' error message
            return;
        }

        if (listing.empty()){
            // we will assume this isn't the user's fault and report that there might be a mistake on the repository etc...
        } else if (listing.find(*library) == listing.end()){
            // user error, the library name is probably wrong.
        }

        if (!fs::exists(nlm + "/temp")) fs::create_directory(nlm + "/temp");


        if (git_libgit2_init()){
            // report init failure and leave
        }

        git_clone_options libgit_opts = GIT_CLONE_OPTIONS_INIT;
        git_repository* repo = nullptr;

        if (git_clone(&repo, listing[*library]["url"].get<std::string>().c_str(), (nlm + "/temp").c_str(), &libgit_opts)){
            std::cerr << "Error cloning repository: " << git_error_last()->message << '\n';
            return;
        }

        git_libgit2_shutdown();

        rw(nlm + "/temp"); // allow file moves
        if (!fs::exists(nlm + "/libs/" +listing[*library]["assembler"].get<std::string>())) fs::create_directory(nlm + "/libs/" + listing[*library]["assembler"].get<std::string>());
        
        //test with nesbrette
        if (!listing[*library]["config"].is_null()) {
            // nlm/temp/neslib/[config]
            if (fs::exists(nlm + "/temp/" + listing[*library]["config"].get<std::string>())){
                std::pair<std::string, std::string> _ = {nlm + "/temp/" + listing[*library]["config"].get<std::string>(), nlm + "/config/" + listing[*library]["name"].get<std::string>()};
                mv(_);
                std::cout << R"(this library uses configuration files, in order to configure a library ensure that you use : './nlm --configure path/to/proj/libs libname'
then ensure that you code like the below:
; include lib dependencies
.include "configs/libname"

; include lib
.include "assembler/libname.asm"
)";
            }
        }


        // installation process:
        
        // target source file within path
        // should work even if source is a path
        std::string _tarpath = nlm + "/libs/" + listing[*library]["assembler"].get<std::string>() + '/' + listing[*library]["name"].get<std::string>();
        //if (!fs::exists(_tarpath)) fs::create_directories(_tarpath);
        std::string _srcpath = nlm + "/temp";
        mv({_srcpath, _tarpath});
    }
}

inline void mv(std::pair<std::string, std::string> task) {
    fs::path source(task.first);
    fs::path target(task.second);
    std::error_code ec;
    fs::rename(source, target, ec);
    if (ec) std::clog << ec << '\n';
}

void rw(const std::string& tar) {
    #ifdef _WIN32
        system(std::format("attrib -r -h {}/*.* /s", tar).c_str());
    #else
        system(("chmod -R -v a-w " + tar).c_str()); // i wish linux luck
    #endif
}