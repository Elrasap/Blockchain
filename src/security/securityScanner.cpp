#include "security/security_scanner.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include "release/checksummer.hpp"
#include "release/verifier.hpp"
#include "release/signer.hpp"

static std::string trim(const std::string& s){size_t a=s.find_first_not_of(" \t\r\n");size_t b=s.find_last_not_of(" \t\r\n");if(a==std::string::npos)return "";return s.substr(a,b-a+1);}
static bool endsWith(const std::string& s,const std::string& suf){return s.size()>=suf.size()&&s.compare(s.size()-suf.size(),suf.size(),suf)==0;}

SecurityScanner::SecurityScanner(const std::string& releaseDir):dir(releaseDir){}

std::string SecurityScanner::findFile(const std::string& suffix) const {
    std::vector<std::string> candidates={
        dir+"/blockchain_node_v1.9.0.bin",
        dir+"/blockchain_node.bin",
        dir+"/node.bin"
    };
    for(auto& c:candidates){
        if(endsWith(c,suffix)) return c;
    }
    std::ifstream f1(dir+"/blockchain_node_v1.9.0.bin"+suffix, std::ios::binary); if(f1) return dir+"/blockchain_node_v1.9.0.bin"+suffix;
    std::ifstream f2(dir+"/blockchain_node.bin"+suffix, std::ios::binary); if(f2) return dir+"/blockchain_node.bin"+suffix;
    std::ifstream f3(dir+"/node.bin"+suffix, std::ios::binary); if(f3) return dir+"/node.bin"+suffix;
    return "";
}

bool SecurityScanner::checkAttestation(const std::string& attestPath, std::string& attestSha, std::string& pubHex) {
    std::ifstream in(attestPath);
    if(!in) return false;
    std::stringstream ss; ss<<in.rdbuf();
    std::string j=ss.str();
    auto findField=[&](const std::string& key)->std::string{
        auto p=j.find("\""+key+"\"");
        if(p==std::string::npos) return "";
        auto c=j.find(":",p);
        if(c==std::string::npos) return "";
        auto q=j.find("\"",c+1);
        if(q==std::string::npos) return "";
        auto r=j.find("\"",q+1);
        if(r==std::string::npos) return "";
        return j.substr(q+1,r-q-1);
    };
    attestSha=findField("sha256");
    pubHex=findField("pubkey");
    std::string sig=findField("signature");
    return !attestSha.empty() && !pubHex.empty() && !sig.empty();
}

bool SecurityScanner::checkChecksum(std::string& outSha, std::string& sbomSha) {
    std::string binPath=findFile(".bin");
    std::string sbomPath=dir+"/sbom.yaml";
    if(binPath.empty()) return false;
    outSha=fileSha256Hex(binPath);
    std::ifstream in(sbomPath);
    if(!in) return false;
    std::string line;
    while(std::getline(in,line)){
        auto L=trim(line);
        if(L.rfind("sha256:",0)==0||L.rfind("sha256",0)==0){
            auto pos=L.find(":");
            if(pos!=std::string::npos){sbomSha=trim(L.substr(pos+1));}
        }
    }
    if(sbomSha.size()==0) return false;
    sbomSha.erase(std::remove(sbomSha.begin(),sbomSha.end(),'\"'),sbomSha.end());
    return outSha==sbomSha;
}

bool SecurityScanner::checkSignature(const std::string& binPath, const std::string& sigPath, const std::string& pubHex) {
    std::ifstream s(sigPath, std::ios::binary);
    if(!s) return false;
    std::stringstream ss; ss<<s.rdbuf();
    std::vector<uint8_t> sig=fromHex(ss.str());
    std::vector<uint8_t> pub=fromHex(pubHex);
    if(sig.empty()||pub.empty()) return false;
    return verifySignatureOverFile(binPath, sig, pub);
}

bool SecurityScanner::checkSbom(const std::string& sbomPath, std::vector<std::string>& deps, std::vector<std::string>& vulns) {
    std::ifstream in(sbomPath);
    if(!in) return false;
    std::string line, name, version;
    while(std::getline(in,line)){
        auto L=trim(line);
        if(L.rfind("name:",0)==0){name=trim(L.substr(5));}
        else if(L.rfind("version:",0)==0){version=trim(L.substr(8)); if(!name.empty()){deps.push_back(name+":"+version); name.clear();}}
    }
    for(auto& d:deps){
        if(d.find("0")!=std::string::npos && d.find(":0")==std::string::npos && d.find(":0.")!=std::string::npos) vulns.push_back("LOW:"+d);
        if(d.find("openssl:")!=std::string::npos && d.find("3.")==std::string::npos) vulns.push_back("CRITICAL:openssl-outdated");
    }
    return !deps.empty();
}

ScanResult SecurityScanner::run() {
    ScanResult r{};
    std::string bin=findFile(".bin");
    std::string sig=findFile(".bin.sig");
    std::string sbom=dir+"/sbom.yaml";
    std::string attest=dir+"/attestation.json";
    std::string sha, sbomSha, attestSha, pubHex;
    r.attestationValid=checkAttestation(attest,attestSha,pubHex);
    r.checksumValid=checkChecksum(sha,sbomSha);
    if(r.attestationValid && r.checksumValid && sha!=attestSha) r.checksumValid=false;
    r.signatureValid=checkSignature(bin,sig,pubHex);
    std::vector<std::string> deps;
    r.sbomValid=checkSbom(sbom,deps,r.vulnerabilities);
    return r;
}

