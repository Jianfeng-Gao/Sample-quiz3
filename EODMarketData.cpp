#include <cstring>
#include <iostream>  
#include <vector>
#include <iomanip>
#include <fstream>
#include "curl/curl.h"
#include <limits>

using namespace std;
const char* cIWV3000SymbolFile = "iShares-Russell-3000-ETF_fund.csv";

void populateSymbolVector(vector<string>& symbols)
{
    ifstream fin(cIWV3000SymbolFile);
    string line;
    getline(fin, line); // skip header
    string symbol;
    while (getline(fin, line)) {
        stringstream sin(line);
        getline(sin, symbol, ',');
        if (!symbol.empty())
            symbols.push_back(symbol);
    }
}

int write_data(void* ptr, int size, int nmemb, FILE* stream)
{
	size_t written;
	written = fwrite(ptr, size, nmemb, stream);
	return written;
}

struct MemoryStruct {
	char* memory;
	size_t size;
};

void* myrealloc(void* ptr, size_t size)
{
	if (ptr)
		return realloc(ptr, size);
	else
		return malloc(size);
}

int write_data2(void* ptr, size_t size, size_t nmemb, void* data)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct* mem = (struct MemoryStruct*)data;
	mem->memory = (char*)myrealloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory) {
		memcpy(&(mem->memory[mem->size]), ptr, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;
	}
	return realsize;
}

// Helper function to read API token from a file
string read_api_token(const string& filename) {
    ifstream fin(filename);
    string token;
    if (fin.is_open()) {
        getline(fin, token);
        fin.close();
    } else {
        throw runtime_error("Failed to open API token file: " + filename);
    }
    return token;
}

size_t find_nth_of(const string& s, char ch, int n) {
    size_t pos = -1;
    while (n-- > 0) {
        pos = s.find(ch, pos + 1);
        if (pos == string::npos) return string::npos;
    }
    return pos;
}

int main(void)
{
    try {
        vector<string> symbolList;
        symbolList.push_back("MSFT");
	
        const string api_token_file = "api_token.txt"; // Place your API key in this file

        // Read API token from file
        string api_token = read_api_token(api_token_file);

        CURL* handle;
        CURLcode status;

        curl_global_init(CURL_GLOBAL_ALL);
        handle = curl_easy_init();

        if (!handle) {
            throw runtime_error("Curl init failed!");
        }

        string url_common = "https://eodhistoricaldata.com/api/eod/";
        string start_date = "2025-11-01";
        string end_date = "2025-11-31";

        for (const auto& symbol : symbolList)
        {
            struct MemoryStruct data;
            data.memory = NULL;
            data.size = 0;

            string url_request = url_common + symbol + ".US?" + "from=" + start_date + "&to=" + end_date + "&api_token=" + api_token + "&period=d";
            curl_easy_setopt(handle, CURLOPT_URL, url_request.c_str());

            curl_easy_setopt(handle, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:74.0) Gecko/20100101 Firefox/74.0");
            curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0);
            curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0);
            curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data2);
            curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*)&data);

            status = curl_easy_perform(handle);

            if (status != CURLE_OK) {
                free(data.memory);
                throw runtime_error(string("curl_easy_perform() failed: ") + curl_easy_strerror(status));
            }
            stringstream sData;
            sData.str(data.memory);
            string MinOpenDate, MaxCloseDate;
            string sOpen,sClose;
            string sDate;
            double max=0;
            double min=std::numeric_limits<double>::infinity();
            string line;
            cout <<"Symbol="<<symbol << endl;
            while (getline(sData, line)) {
                size_t found = line.find('-');
                if (found != std::string::npos)
                {
                    cout<<line<<endl;
                    sDate=line.substr(0,line.find_first_of(','));
                    sOpen=line.substr(line.find_first_of(',')+1,find_nth_of(line,',',2));
                    sClose=line.substr(find_nth_of(line,',',4)+1,find_nth_of(line,',',5));
                    if(strtod(sClose.c_str(),NULL)>max)
                    {
                        max=strtod(sClose.c_str(),NULL);
                        MaxCloseDate=sDate;
                    }
                    if(strtod(sOpen.c_str(),NULL)<min)
                    {
                        min=strtod(sOpen.c_str(),NULL);
                        MinOpenDate=sDate;
                    }
                }
            }
            cout<<"sMinOpenDate="<<MinOpenDate<<", "<<std::fixed<<::setprecision(3)<<"dMinOpenPrice="<<min<<endl;
            cout<<"sMaxCloseDate="<<MaxCloseDate<<", "<<std::fixed<<::setprecision(3)<<"dMaxClosePrice="<<max<<endl;
            free(data.memory);
        }

        curl_easy_cleanup(handle);
        curl_global_cleanup();
    }
    catch (const exception& ex) {
        cerr << "Exception: " << ex.what() << endl;
        return -1;
    }
    catch (...) {
        cerr << "Unknown exception occurred." << endl;
        return -1;
    }

    return 0;
}
