#include "release/releasePublisher.hpp"
#include "release/releaseVerifier.hpp"
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <sstream>

using json = nlohmann::json;

static bool uploadFile(const std::string& url,
                       const std::string& token,
                       const std::string& filePath,
                       const std::string& mime) {
    std::ifstream f(filePath, std::ios::binary);
    if(!f){ std::cerr<<"Missing "<<filePath<<"\n"; return false; }
    std::ostringstream ss; ss<<f.rdbuf();
    std::string data=ss.str();

    CURL* curl=curl_easy_init();
    if(!curl) return false;
    struct curl_slist* headers=nullptr;
    std::string auth="Authorization: token "+token;
    headers=curl_slist_append(headers, auth.c_str());
    headers=curl_slist_append(headers, ("Content-Type: "+mime).c_str());
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());
    CURLcode res=curl_easy_perform(curl);
    bool ok=(res==CURLE_OK);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    std::cout<<(ok?"[OK] Uploaded ":"[FAIL] Upload " )<<filePath<<"\n";
    return ok;
}

bool ReleasePublisher::publish(const std::string& manifestPath,
                               const std::string& repo,
                               const std::string& tag,
                               const std::string& token) {
    json m;
    std::ifstream in(manifestPath);
    if(!in){ std::cerr<<"Missing manifest\n"; return false; }
    in>>m;
    std::string api="https://api.github.com/repos/"+repo+"/releases";
    std::string createJson=json({{"tag_name",tag},{"name",tag},{"body","Automated blockchain release"}}).dump();

    CURL* curl=curl_easy_init();
    if(!curl) return false;
    struct curl_slist* h=nullptr;
    std::string auth="Authorization: token "+token;
    h=curl_slist_append(h,auth.c_str());
    h=curl_slist_append(h,"Content-Type: application/json");
    curl_easy_setopt(curl,CURLOPT_URL,api.c_str());
    curl_easy_setopt(curl,CURLOPT_HTTPHEADER,h);
    curl_easy_setopt(curl,CURLOPT_POSTFIELDS,createJson.c_str());
    std::ostringstream resp;
    curl_easy_setopt(curl,CURLOPT_WRITEDATA,&resp);
    curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,+[](char*p,size_t s,size_t n,void*d){
        ((std::ostringstream*)d)->write(p,s*n);return s*n;});
    CURLcode res=curl_easy_perform(curl);
    curl_slist_free_all(h);
    curl_easy_cleanup(curl);
    if(res!=CURLE_OK){std::cerr<<"Release creation failed\n";return false;}

    auto jr=json::parse(resp.str());
    if(!jr.contains("upload_url")){std::cerr<<"Invalid response\n";return false;}
    std::string uploadUrl=jr["upload_url"];
    size_t q=uploadUrl.find('{'); if(q!=std::string::npos) uploadUrl=uploadUrl.substr(0,q);

    bool ok=true;
    ok&=uploadFile(uploadUrl+"?name=blockchain_node.bin",token,m["binary"],"application/octet-stream");
    ok&=uploadFile(uploadUrl+"?name=release_manifest.json",token,manifestPath,"application/json");
    if(m.contains("sbom")) ok&=uploadFile(uploadUrl+"?name=sbom.yaml",token,m["sbom"],"text/yaml");
    if(m.contains("attestation")) ok&=uploadFile(uploadUrl+"?name=attestation.json",token,m["attestation"],"application/json");
    return ok;
}

