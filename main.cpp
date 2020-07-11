#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <curl/curl.h> // libcurl install required
#include "picojson.h" // picojson required

using namespace std;

size_t onReceive(char* ptr, size_t size, size_t nmemb, void* stream){
  std::vector<char>* recvBuffer = (std::vector<char>*)stream;
  const size_t sizes = size * nmemb;
  recvBuffer->insert(recvBuffer->end(),(char*)ptr, (char*)ptr + sizes);
  return sizes;
}

const char* acag2jcc(const char* acag){
  ifstream fin("acag.dat");

  if(!fin.is_open()){
    std::cout << "couldn't open dat file";
    return "-";
  }

  string oneLine;
  while (getline(fin,oneLine)){
    istringstream iss(oneLine);
    string jcc,cityname;
    iss >> jcc >> cityname;
    if(cityname == acag){
      fin.close();
      return jcc.c_str();
    }
  }
  fin.close();
  return "-";

}


int main(){
  const char* prefixs[22] = {"JA","JD","JE","JF","JG","JH","JI","JJ","JK","JL","JM","JN","JO","JP","JQ","JR","JS","7J","7K","7L","7M","7N"};
  const char* suf1st[24] = {"A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X"};
        // curl setting
        CURL *curl = curl_easy_init();
        if(curl == nullptr){
          curl_easy_cleanup(curl);
          std::cout << "Error occurred during CURL init." <<std::endl;
          return 1;
        }


  for (int i=0;i<22;i++){
    for (int j=0;j<10;j++){
      for(int k=0;k<24;k++){

        ofstream fout("out.spc", std::ios::app); // open .spc file

        if(!fout.is_open()){
          std::cout << "couldn't open outfile." << std::endl;
          return 1;
        }

        // send HTTP request
        string baseURL = "https://www.tele.soumu.go.jp/musen/list?ST=1&OF=2&DA=0&DC=3&SC=0&OW=AT&MA=";

        string URL = baseURL + (string)prefixs[i] + std::to_string(j) + (string)suf1st[k];//stringで結合

        std::vector<char> responseData;

        curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER,0);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onReceive);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

        CURLcode res = curl_easy_perform(curl);
        std::cout << "sent HTTP request of " << prefixs[i] << j << suf1st[k]<< endl;
        if (res != CURLE_OK){
          std::cout << "curl_easy_perform failed." << curl_easy_strerror(res) << std::endl;
          curl_easy_cleanup(curl);
          return 1;
        }


        // parse JSON
        picojson::value v;
        std::string err;
        picojson::parse(v, responseData.data(), responseData.data() + responseData.size(), &err);
        if(err.empty()){
          std::cout << "Parsing JSON of "<< prefixs[i] << j << suf1st[k] <<endl;
          std::string num = v.get<picojson::object>()["musenInformation"].get<picojson::object>()["totalCount"].get<std::string>();

          if (std::atoi(num.c_str()) != 0){ // skip non-existing prefixes
            picojson::array musens = v.get<picojson::object>()["musen"].get<picojson::array>();
            for(picojson::array::iterator it = musens.begin(); it != musens.end(); it++){
              std::string callsign = it->get<picojson::object>()["listInfo"].get<picojson::object>()["name"].get<std::string>();
              std::string acag = it->get<picojson::object>()["listInfo"].get<picojson::object>()["tdfkCd"].get<std::string>();
              fout << callsign.substr(18,6).c_str()<< " " <<acag2jcc(acag.c_str()) << endl;
            }
            std::cout << "Written " << prefixs[i] << j << suf1st[k] << " to spc." <<endl;
          }
          fout.close();
        }
        sleep(1);// wait 1 sec in order not to "attack" API
      }
    }
  }
  curl_easy_cleanup(curl);
  return 0;
}
