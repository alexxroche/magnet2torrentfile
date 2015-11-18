#include <cstdlib>
#include <sstream>      // stringstream
#include <iostream>     // toupper
#include <algorithm>    // find
#include <ctype.h>      // toupper
#include <unistd.h>     // isatty
#include <curl/curl.h>  // curl

//using namespace std;

#define DEBUG 0

/* may be used by libcurl to write the torrent file to disk */
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

//extern char **environ;

int main(int argc, char *argv[])
{
	std::string f_path;
	if(const char* env_p = std::getenv("USERPROFILE"))
		std::string f_path = env_p + f_path;

    FILE *log;
    if (DEBUG){
            std::string logfile = f_path + "\\workspace\\C++\\magnet2torrentfile\\bin\\Debug\\m2tf.log";
            log = fopen(logfile.c_str(),"a");
            fprintf(log,"LOG: ===int main===\n");
    }

    if(isatty(STDIN_FILENO))
    {
        if(argv[1]){
            std::string mag_str = std::string(argv[1]);
            while (std::size_t pos = mag_str.find(':') )
            {
                std::string match = mag_str.substr(0, pos);
                if(DEBUG && match.compare("magnet") == 0 ){
                        printf("this looks like a valid magnet %s\n", match.c_str());
                }
                pos +=1;
                std::size_t before_truncation = mag_str.length();
                mag_str = mag_str.substr(pos);
                if(before_truncation == mag_str.length())
                   break;
            } //while
            while (std::size_t pos = mag_str.find('&') )
            {
                std::string match = mag_str.substr(0, pos);
                if(std::size_t kvd = match.find('=')) // key value delimiter
                {
                    std::string key = match.substr(0, kvd);
                    std::string value = match.substr(kvd+1);
                    if(DEBUG && key != value){
                            printf("%s = %s\n", key.c_str(), value.c_str());
                    }
                    else if(match.length() == 40 )
                    {
                        if (DEBUG >=2)
                        {
                            fprintf(log,"LOG: match = %s\n", match.c_str());
                        }

                        char magChar[40];
                        for(int i = 0; i < int(match.length()); i++)
                            magChar[i] = toupper(match[i]);

                        // C++ really does not like c_str so we have to
                        // FORCE it to strip any extra chars from the end!!

                        std::string magHash = magChar;
                        magHash = magHash.substr(0, 40);

                        if (DEBUG >=2)
                        {
                            fprintf(log,"LOG: Good magHash = %s\n", magHash.c_str());
                        }

                        /*
                        if (DEBUG)
                        {
                            fprintf(log,"LOG: magHash = %s\n", magHash);
                        }
                        */

                        //printf("Lets ask https://torcache.net/torrent/%s.torrent\n", magHash);
                        FILE *fp;
                        CURL *curl = curl_easy_init();
                        if(curl)
                        {
                            std::string torrentoutPath = "."; // get this from m2tf.cfg if available
                             // turns out that .\ on windows does not work like ./ on linux
                            char ownPath[MAX_PATH];
                             // Will contain exe path
                            HMODULE hModule = GetModuleHandle(NULL);
                            if (hModule != NULL)
                            {
                                // When passing NULL to GetModuleHandle, it returns handle of exe itself
                                GetModuleFileName(hModule,ownPath, (sizeof(ownPath)));
                                std::string pwd = std::string(ownPath);

                                #if defined(_WIN32) || defined(WIN32)
                                    int path_end = pwd.rfind('\\');
                                #else
                                    int path_end = pwd.rfind('/');
                                #endif

                                //    int path_end = pwd.rfind('\\');
                                //PathRemoveFileSpec(ownPth);
                                std::string ownPth = pwd.substr(0,path_end+1);
                                torrentoutPath = ownPth;
                            }

                            if (DEBUG)
                            {
                                fprintf(log,"LOG: Good magHash = %s\n", magHash.c_str());
                            }
                            std::stringstream outFileStream;
                            if (DEBUG)
                            {
                                fprintf(log,"LOG:  Bad magHash = %s\n", magHash.c_str());
                            }

                            //std::string RmagHash = magHash.c_str(); // Debug worked, but Release didn't
                            outFileStream << torrentoutPath; // << magHash << ".torrent";
                            std::string outfilenameStr = outFileStream.str() + magHash + ".torrent";
                            char * outfilename = strdup(outfilenameStr.c_str());

                            if (DEBUG)
                            {
                                fprintf(log,"LOG: outfilename = %s\n", outfilename);
                            }
                            fp = fopen(outfilename,"wb");
                            // check we can write
                            if (fp==NULL){
                                fprintf(stderr,"ERR: unable to create functioning filehandle for %s", outfilename);
                            }
                            else if(DEBUG)
                            {
                                printf("created filehandel for new .torrent file at %s", outfilename);
                            }
                            CURLcode res;
                            std::stringstream myURIstream;
                            myURIstream << "https://torcache.net/torrent/" << magHash << ".torrent";
                            std::string myURIstr = myURIstream.str();
                            char * myURI = strdup(myURIstr.c_str());

                            if (DEBUG)
                                fprintf(log, "Converting:\n %s\n to:\n %s\n", myURI, outfilename);

                            if (DEBUG >=1)
                                curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

                            curl_easy_setopt(curl, CURLOPT_URL, myURI);
                            curl_easy_setopt(curl, CURLOPT_HEADER, 0L); // superflous
                            curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, ""); //gunzip
                            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
                            //curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL); //this also works
                            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
                              //FILE *capath = ".\ca-bundle.crt";
                              //curl_easy_setopt(curl, CURLOPT_CAPATH, capath);
                            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
                              // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
                            res = curl_easy_perform(curl);
                            if(res != CURLE_OK){
                                fprintf(stderr, "curl_easy_perform() failed: %s\n",
                                curl_easy_strerror(res));
                                Sleep(876);
                            }
                            /*else if(DEBUG)
                            {
                                printf("cURL says: %s\n", curl_easy_strerror(res));
                            }
                            */
                            curl_easy_cleanup(curl);
                            fclose(fp);
                        } //if(curl)

                    }
                    else if(DEBUG)
                    {
                        printf("... and more cruft: %s\n", match.c_str());
                    } // if we have a guess what we have found

                }
                pos +=1;
                std::size_t before_truncation = mag_str.length();
                mag_str = mag_str.substr(pos);
                if(before_truncation == mag_str.length())
                    break; // it isn't getting any shorter
            } //while we search through for key=value pairs

            // extract filename (if we have one) or the hash
            // create $filename.torrent

            // make http request for this hash
            //  - if we could make a DHT/peer request we should
            // wait for reply
            // write .torrent to file

            fclose(stdout);
        }
        else
        {
            // we have no input arguments
            // so we want to call the gui function
            // but for now we will just create an error file
                freopen("m2tf.err" ,"w",stdout);
                printf("dude! where's your magnet at? read m2tf.cfg");
                fclose(stdout);
        }

    }
    if(DEBUG){
        fclose(log);
        Sleep(1567);
    }
    return(0);
}

