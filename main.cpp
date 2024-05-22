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

const std::string nlm              = std::string("c:/nlm");     // MIGRATE TO SYSTEM ENVIRONMENT VARIABLE (install process change?)

namespace fs = std::filesystem;
using namespace nlohmann;

CURL *curl;
CURLcode curl_code = CURLE_OK;

char CURL_ERROR_BUFFER[0];

inline void unzip(std::string& target);
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

    curl = curl_easy_init();
    if (!curl){
        // bla bla bla error
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, CURL_ERROR_BUFFER);

    // check for files existing
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

        curl_easy_setopt(curl, CURLOPT_URL, target_url.c_str());
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
        if(!curl) {
            return {};
        } else {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, net::WriteCallback_nofstream);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
            curl_code = curl_easy_perform(curl);
            if(curl_code != CURLE_OK) {
                curl_easy_cleanup(curl);
                return {};
            }
            curl_easy_cleanup(curl);
        }
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
        if (!library->contains('/')){   // MIGRATE TO CLI HANDLER
            // error
            return;
        }

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

        // otherwise pull library, via curl  (maybe git later idk)

        json _library = json::parse(net::download(*library));
        
        // installation process:
        if (!fs::exists(nlm + "/temp/")) fs::create_directory(nlm + "/temp");
        if (net::download(_library["url"], nlm + "/temp")){
            // error cant get file
            fs::remove_all(nlm + "/temp");
        }
        // deduct filename
        std::string filename = _library["url"].get<std::string>().substr(_library["url"].get<std::string>().find_last_of('/'+1));

        // install assembler path if assembler path not installed
        if (!fs::exists(nlm + "/libs/" + _library["assembler"].get<std::string>())) fs::create_directory(nlm + "/libs" + _library["assembler"].get<std::string>());
        
        // create lvalue for unzip method (may adapt)
        std::string _ = fs::path(nlm + "/temp/" + filename).string();
        unzip(_);

        fs::remove(nlm + "/temp/" + filename); // delete zip
        filename = filename.substr(0,filename.find_last_of(".")) + '/'; // folder name is based of filename

        if (_library["config"] != "null")
            // nlm/temp/neslib/[config]
            if (fs::exists(nlm + "/temp/" + filename + _library["config"].get<std::string>())){
                std::pair<std::string, std::string> _ = {nlm + "/temp/" + _library["config"].get<std::string>(), nlm + "/config/" + _library["name"].get<std::string>()};
                mv(_);
                std::cout << R"(this library uses configuration files, in order to configure a library ensure that you use : './nlm --configure path/to/proj/libs libname'
then ensure that you code like the below:
; include lib dependencies
.include "configs/libname"

; include lib
.include "assembler/libname.asm"
)";
            } else{
                std::cerr << "cannot copy config file for " << _library["name"].get<std::string>() << ", config path cannot be found.\n";
                return;
            }

        // target source file within path
        // should work even if source is a path
        std::string _tarpath = nlm + "/libs/" + _library["assembler"].get<std::string>() + _library["name"].get<std::string>();
        std::string _srcpath = nlm + "/temp/" + filename + _library["source"].get<std::string>();
        mv({_srcpath, _tarpath});
    
    }
}


inline void unzip(std::string& target) {
    #ifdef _WIN32
    system(("tar -xvf " + target).c_str());
    #else
    system(("unzip " + target).c_str());
    #endif
    return;
}

inline void mv(std::pair<std::string, std::string> task) {
    fs::path source(task.first);
    fs::path target(task.second);
    fs::rename(source, target);
}