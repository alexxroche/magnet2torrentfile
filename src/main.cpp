#include <iostream>     // cout toupper
#include <cstdlib>      // getenv
#include <unistd.h>     // isatty
#include <sstream>      // ostringstream
#include <algorithm>    // find
#include <ctype.h>      // toupper
#include <string>       // .replace
#include <ctime>
// would it be nice to compile libcurl.dll
// or would it be even better if we could strip it down
// to what we need and compile it into the binary?
#include <lib/curl/curl.h>  // curl

#include <sys/types.h>  // stat (to check if dir exists)
#include <sys/stat.h>   // stat (to check if dir exists)


//gcc  -DINI_NOBROWSE=1 -o obj/minIni.o -c lib/minini/minIni.c //<<< pre-compile
// -L obj/minIni.o
#include "lib/minini/minIni.h"     // config parser

#define version "0.3" //20151123 17:11
#define cfg_file "m2tf.cfg"
#define APPNAME "magnet2torrentfile"
#define FALSE 0
#define TRUE 1

// creating an ico
// gcc -E -xc -DRC_INVOKED main.rc -o main.res

#define DEBUG FALSE
#define LOG FALSE

#ifdef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) fprintf(stderr, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif


// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime() {
    //http://stackoverflow.com/a/10467633/1153645
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%dT%X", &tstruct);
    //strftime(buf, sizeof(buf), "%FT%TZ", &tstruct);

    return buf;
}



/* keep those strings trim! */
std::string trim(const std::string &s)
{
    /* http://stackoverflow.com/a/21698913/1153645 */
    std::string::const_iterator it = s.begin();
    while (it != s.end() && isspace(*it))
        it++;

    std::string::const_reverse_iterator rit = s.rbegin();
    while (rit.base() != it && isspace(*rit))
        rit++;

    return std::string(it, rit.base());
}

/* debug */
void GOT_HERE(std::string s)
{
    if(s != "")
        std::cout << s;
    else
        std::cout << "GOT HERE";
    std::cout << "\n";
    system("pause");
    exit(0);
}

/* may be used by libcurl to write the torrent file to disk */
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

/* evil global to hold the X-Transmission-Session-Id: header */
std::string XTSI;

/* used by libcurl to extract the header that we want */
static size_t header_callback(char *buffer, std::size_t size,
                              std::size_t nitems, void *userdata)
{
  /* received header is nitems * size long in 'buffer' NOT ZERO TERMINATED */
    std::string curl_res = buffer;
    std::string XTSIdHeader = "X-Transmission-Session-Id: ";
    if (curl_res.compare(0,27,XTSIdHeader) == 0)
            XTSI = trim(curl_res);
    return nitems * size;
}

int main(int argc, char *argv[])
{

    // lets find a config file (if there is one)

  	std::string cfg_path;
    std::string f_path = "C:\\"; //I feel dirty using backslash in a path name where the first is an escape and the second isn't
	if(const char* env_up = std::getenv("USERPROFILE"))
        f_path = env_up;
	if(const char* appdata = std::getenv("APPDATA"))
		cfg_path = appdata;
	// If it is missing we fall back to pwd of the executable
	cfg_path = cfg_path + "\\" APPNAME;


    if(DEBUG>=2)
        printf("looking for config in AppData %s\\%s\n", cfg_path.c_str(), cfg_file);

    struct stat info;
    char *pathname = strdup(cfg_path.c_str());

    FILE *log;
    if (DEBUG || LOG){
            std::string logfile = f_path + "\\m2tf.log";
            log = fopen(logfile.c_str(),"a");
            if(!log)
            {
                printf("Failed to open logfile: %s", logfile.c_str());
                GOT_HERE("LOG CREATION FAIL");
            }

            fprintf(log,"%s ===Log start main()===\n", currentDateTime().c_str());
    }

    int we_should_read_cfg_from_pwd = FALSE;

    if( stat( pathname, &info ) != 0 )
    {
        //printf( "cannot access %s\n", pathname );
        we_should_read_cfg_from_pwd = TRUE;
    }
    else if( info.st_mode & S_IFDIR )  // S_ISDIR() doesn't exist on my windows
    {
        //check that we actually HAVE a config file!

        we_should_read_cfg_from_pwd = FALSE;
        //printf( "%s is a directory\n", pathname );
    }
    else
    {
        //printf( "%s is no directory\n", pathname );
        we_should_read_cfg_from_pwd = TRUE;
    }

    if(!we_should_read_cfg_from_pwd)
    {
        if(DEBUG)
            std::cout << "USING cfg file in " << cfg_path << "\\" << cfg_file << "\n";
    }

    if(we_should_read_cfg_from_pwd)
    {
        if(DEBUG)
            std::cout << "DID NOT find cfg file in " << cfg_path << "\\" << cfg_file;

        // lets find out where this is running from

        char ownPath[MAX_PATH]; // could we just use %CD% on Win32 ?
        std::string pwd;
        HMODULE hModule = GetModuleHandle(NULL);
        if (hModule != NULL)
        {
            // When passing NULL to GetModuleHandle, it returns handle of exe itself
            GetModuleFileName(hModule,ownPath, (sizeof(ownPath)));
            pwd = std::string(ownPath);

            // here we are searching for the last path/file divider
            // we could also just remove the filename and 1 or 2 chars
            #if defined(_WIN32) || defined(WIN32)
                int path_end = pwd.rfind('\\');
            #else
                int path_end = pwd.rfind('/');
            #endif

            //    int path_end = pwd.rfind('\\');
            //PathRemoveFileSpec(ownPth);
            std::string ownPth = pwd.substr(0,path_end+1);
            std::string bin_name = pwd.substr(path_end+1,pwd.size());
            printf("%s ", bin_name.c_str());
            pwd = ownPth;
        }
        else
        {
            if(DEBUG)
                printf("Where's your handel?\n");
        } // if (hModule != NULL)
        if(DEBUG)
            printf("running from %s\n", pwd.c_str());

        cfg_path = pwd;
    }
    //std::string pwd = pwd();
    //std::string pwd = "C:\\Users\\alexx\\workspace\\C++\\magnet2transmission";
   //char cwd[MAX_PATH];
   //cwd = sprintf("%s", pwd);
    //parse_ini(pwd, "m3tf.cfg");
    //std::string inifile = "m2tf.cfg";
  //   minIni ini(pwd.c_str() + inifile);
    if(DEBUG)
        std::cout << "reading " << cfg_path << "\\" << cfg_file << " (if it exists, else we fall back to defaults)\n";

     minIni ini(cfg_path+"\\"+cfg_file);
     int tf_enabled, nas_enabled;
     tf_enabled = ini.geti("torrentfile", "tf_enabled", 1);
     nas_enabled = ini.geti("NAS", "nas_enabled", 0);

	if(isatty(STDIN_FILENO))
    {
        //fprintf(log,"%s it IS a tty!", currentDateTime().c_str());
        if(argv[1])
        {
            //fprintf(log,"%s argv[1] %s", currentDateTime().c_str(), argv[1]);
            std::string mag_str = std::string(argv[1]);

            if(argc == 2 && mag_str.compare("-v") == 0)
            {
                // this does not work if compiled with -mwindows
                std::cout << "Version: " << version << std::endl;
                exit(0);
            }
            int valid_magnet = 0;
            while (std::size_t pos = mag_str.find(':') )
            {
                std::string match = mag_str.substr(0, pos);
                if(match.compare("magnet") == 0 ){
                        //printf("this looks like a valid magnet %s\n", match.c_str());
                        valid_magnet = 1;
                        // nas just needs the magnet, so if that's all we are doing
                        // we can stop parsing here
                        if (!tf_enabled || tf_enabled != 1 )
                            break; // and we will do the m2transmission if that is enabled
                }
                pos +=1;
                std::size_t before_truncation = mag_str.length();
                mag_str = mag_str.substr(pos);
                if(before_truncation == mag_str.length())
                   break;
            } //while
            // by now we should know if this looks like a valid magnet link
            if(!valid_magnet){
                fprintf(stderr,"Err: Does not look like a valid magnet\n");
                if(LOG)
                    fprintf(log,"%s failed to find valid magnet", currentDateTime().c_str());
                exit(3);
            }

            if(LOG)
                fprintf(log,"%s checking for tf_enabled = %d\n", currentDateTime().c_str(), tf_enabled);

        if(tf_enabled == 1){
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

                        // grab the config
                         std::string tf_out_path;
                         tf_out_path = ini.gets("torrentfile", "tf_out_path", "C:\\");
                         std::string m2tf_site1_enabled, m2tf_site1_name, m2tf_site1_type, m2tf_site1_uri;
                         m2tf_site1_enabled = ini.geti("torrentfile", "m2tf_site1_enabled", 1);
                         m2tf_site1_name = ini.gets("torrentfile", "m2tf_site1_name", "site1");
                         m2tf_site1_type = ini.gets("torrentfile", "m2tf_site1_type", "http_get");
                         /* we might need to have http_form and http_login+http_form or http_login+http_get */
                         m2tf_site1_uri = ini.gets("torrentfile", "m2tf_site1_uri", "https://example.com/%s.torrent");
                         if(DEBUG>=2)
                            std::cout << "We want to splice in the MAGNET to the uri: " << m2tf_site1_uri << "\n";

                         if(std::size_t mag_pos = m2tf_site1_uri.find("%s"))
                         {
                             //std::string mag_str = magHash;
                             /* this does not work with large magnet strings
                             nas_str.replace(mag_pos,mag_pos+1,mag_str);
                             */
                             std::string m2tf_site1_uri_left_of_anchor = m2tf_site1_uri.substr(0,mag_pos);
                             // mag_pos+length_of_matching_string("%s")
                             std::string m2tf_site1_uri_right_of_anchor = m2tf_site1_uri.substr(mag_pos+2,m2tf_site1_uri.size());
                             std::ostringstream m2tf_site1_uri_with_mag;
                             m2tf_site1_uri_with_mag << m2tf_site1_uri_left_of_anchor << magHash << m2tf_site1_uri_right_of_anchor;
                             std::string m2tf_site1_uri_new = m2tf_site1_uri_with_mag.str();
                             m2tf_site1_uri = m2tf_site1_uri_new;
                             //std::cout << nas_str;
                         }

                         if(DEBUG>=1)
                            std::cout << "URI+MAGNET: " << m2tf_site1_uri << "\n";

                        //printf("Lets ask https://torcache.net/torrent/%s.torrent\n", magHash);
                        FILE *fp;
                        CURL *curl = curl_easy_init();
                        if(curl)
                        {
                            std::string torrentoutPath = tf_out_path + "\\"; // get this from m2tf.cfg if available
                            // turns out that .\ on windows does not work like ./ on linux

                            //NTS you are here checking that torrentoutPath exists (or falling back to pwd
                            // if there was every a piece of code that wanted factoring out into its own function....
                            int output_tf_to_pwd = 0;
                            struct stat info;
                            char *pathname = strdup(torrentoutPath.c_str());

                            if( stat( pathname, &info ) != 0 )
                            {
                                //printf( "cannot access %s\n", pathname );
                                output_tf_to_pwd = TRUE;
                            }
                            else if( info.st_mode & S_IFDIR )  // S_ISDIR() doesn't exist on my windows
                            {
                                //check that we actually HAVE an output dir
                                output_tf_to_pwd = FALSE;
                            }
                            else
                            {
                                //printf( "%s is no directory\n", pathname );
                                output_tf_to_pwd = TRUE;
                            }

                            if(output_tf_to_pwd)
                            {
                                // we need to know what the PWD is!
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
                            } // if(output_tf_to_pwd)

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
                            /*
                            std::stringstream myURIstream;
                            myURIstream << "https://torcache.net/torrent/" << magHash << ".torrent";
                            std::string myURIstr = myURIstream.str();
                            char * myURI = strdup(myURIstr.c_str());
                            */

                            char * myURI = strdup(m2tf_site1_uri.c_str());

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
        } // if(tf_enabled == 1)

        //fprintf(log,"nas_enabled == %d\n", nas_enabled);
        //GOT_HERE("TF_done, considering NAS");

        if(nas_enabled == 1)
        {
            std::string nas_prot, nas_user, nas_pass, nas_ip, nas_port, nas_uri, nas_str, nas_post_uri, nas_download_dir;
            //std::string nas_login
            nas_prot = ini.gets("NAS", "nas_prot", "http");
            nas_user = ini.gets("NAS", "nas_user", "admin");
            nas_pass = ini.gets("NAS", "nas_pass", "admin");
            nas_ip   = ini.gets("NAS", "nas_ip", "192.168.10.11");
            nas_port = ini.gets("NAS", "nas_port", "9091");
            nas_download_dir = ini.gets("NAS", "nas_download_dir", "/share/MD0_DATA/Download/transmission/completed");
            //nas_login= ini.gets("NAS", "nas_login", "transmission/web"); // turns out that we don't use this, we trigger a failure to rpc to get the X header!!
            nas_post_uri= ini.gets("NAS", "nas_post_uri", "transmission/rpc");
            nas_str  = ini.gets("NAS", "nas_str", "{\"method\":\"torrent-add\",\"arguments\":{\"filename\":\"%s\",\"download-dir\":\"%s\",\"paused\":false},\"tag\":\"\"}");
     if(DEBUG>=2)
        std::cout << nas_prot << "://" << nas_user << ":" << nas_pass << "@" << nas_ip << ":" << nas_port << "\n";

     nas_uri = nas_prot + "://" + nas_user + ":" + nas_pass + "@" + nas_ip + ":" + nas_port;

            if(DEBUG>=2)
                fprintf(log,"nas_uri = %s\n", nas_uri.c_str());

            if(DEBUG>=1)
                fprintf(log,"%s mag_str = %s\n", currentDateTime().c_str(), mag_str.c_str());


     // lets put the magnet into the json string ready to POST to the server

     if(DEBUG>=2)
        std::cout << "We want to replaced the json holder in: " << nas_str << "\n";

     if(std::size_t mag_pos = nas_str.find("%s"))
     {
         std::string mag_str = std::string(argv[1]);
         // if .find does not locate it will return the size of nas_str... which should throw and error!
         /* this does not work with large magnet strings
         nas_str.replace(mag_pos,mag_pos+1,mag_str);
         */
         std::string nas_str_left_of_anchor = nas_str.substr(0,mag_pos);
         // NTS surely this shoulod be +2 ??? what that blind luck?
         std::string nas_str_right_of_anchor = nas_str.substr(mag_pos+2,nas_str.size());
         std::ostringstream nas_str_with_mag;
         nas_str_with_mag << nas_str_left_of_anchor << mag_str << nas_str_right_of_anchor;
         std::string nas_str_new = nas_str_with_mag.str();
         nas_str = nas_str_new;
         //std::cout << nas_str;
     }
     if(DEBUG)
     {
         fprintf(log,"mas_str now: %s\n", nas_str.c_str());
                    //std::cout << "We have replaced the json holder with mag: " << nas_str << "\n";
     }

     // and the default download dir
     if(std::size_t dldir_pos = nas_str.find("%s"))
     {
         std::string nas_str_left_of_anchor = nas_str.substr(0,dldir_pos);
         std::string nas_str_right_of_anchor = nas_str.substr(dldir_pos+2,nas_str.size());
         std::ostringstream nas_str_with_dldir;
         nas_str_with_dldir << nas_str_left_of_anchor << nas_download_dir << nas_str_right_of_anchor;
         std::string nas_str_new = nas_str_with_dldir.str();
         nas_str = nas_str_new;
        /* this might work in most cases, but we can't take the risk
        std::string dldir_str = nas_download_dir;
         nas_str.replace(dldir_pos,dldir_pos+1,dldir_str);
         */
     }

      //system("pause"); exit(1);
        if(DEBUG)
        {
            fprintf(log,"nas_str = %s\n", nas_str.c_str());
            std::cout << nas_str << "\n";
            //GOT_HERE("check nas_str");
        }

        if(DEBUG || LOG)
            fprintf(log,"%s creating curl for nas\n", currentDateTime().c_str());

            CURL *curl = curl_easy_init();
                if(curl)
                {
                    if(LOG)
                        fprintf(log,"%s have curl4nas\n", currentDateTime().c_str());
                    CURLcode res;
                    // this may seem like overkill, but when we merge back into magnet2torrent it will be needed
                    std::ostringstream myURIstream;
                    //myURIstream << nas_uri << "/" << nas_login << "/";
                    myURIstream << nas_uri << "/" << nas_post_uri << "/";
                    std::string myURIstr = myURIstream.str();
                    char * myURI = strdup(myURIstr.c_str());

                    if (DEBUG >=2)
                        std::cout << "We are going to: " << myURI << "\n";
                    if (DEBUG >=2)
                        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
                    if (DEBUG <=12)
                        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);


                    curl_easy_setopt(curl, CURLOPT_URL, myURI);
                    curl_easy_setopt(curl, CURLOPT_HEADER, 0L); // superflous
                    //curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, ""); //gunzip

                    FILE* devnull = fopen("nul", "w");
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, devnull);
                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
                    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
                    //curl_easy_setopt(curl, CURLOPT_NOBODY, 1); fails to get the session-id!
                    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); //might be needed for login
                    res = curl_easy_perform(curl);
                    if(res != CURLE_OK){
                        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                        curl_easy_strerror(res));
                        system("pause");
                    }
                    else
                    {
                        // lets POST the magnet !!!
                         CURLcode posted;
                        std::ostringstream postURIstream;
                        postURIstream << nas_prot << "://" << nas_ip << ":" << nas_port << "/" << nas_post_uri << "/";
                        std::string postURIstr = postURIstream.str();
                        const char *postURI = postURIstr.c_str();
                        // char* postURI = new char[postURIstr.length() + 1];
                        // std::copy(postURI.begin(), postURI.end(), char);

                        if (DEBUG >=1)
                            std::cout << "We are going to: " << postURI << "\n";

                        curl_easy_setopt(curl, CURLOPT_URL, postURI);
                        curl_easy_setopt(curl, CURLOPT_USERAGENT, "magnet2torrentfile/0.3");

                        curl_easy_setopt(curl, CURLOPT_POST, 1L); // lets cURL know we are going to post
                        struct curl_slist *chunk = NULL;
                        //chunk = curl_slist_append(chunk, "Accept:");
                        //chunk = curl_slist_append(chunk, "Content-Type:");
                        //chunk = curl_slist_append(chunk, "Accept-Encoding: *");
                        //chunk = curl_slist_append(chunk, "Accept-Language: en-US,en;q=0.5");
                        //chunk = curl_slist_append(chunk, "Connection: keep-alive");
                        //chunk = curl_slist_append(chunk, "Cache-Control: no-cache");
                        //chunk = curl_slist_append(chunk, "Pragma: no-cache");
                        chunk = curl_slist_append(chunk, "X-Requested-With: XMLHttpRequest");
                        chunk = curl_slist_append(chunk, XTSI.c_str());
                        chunk = curl_slist_append(chunk, "Accept: application/json, text/javascript, */*; q=0.01");
                        chunk = curl_slist_append(chunk, "Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
                        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

                        const char *data = strdup(nas_str.c_str());
                        const long data_size = nas_str.size();

                        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data_size);
                        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
                        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

                        posted = curl_easy_perform(curl);
                        int httpCode(0);
                        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

                        if(posted != CURLE_OK || httpCode != 200){
                            fprintf(stderr, "\n\ncurl_easy_perform() failed to POST your magnet ;-( %s\n",
                                    curl_easy_strerror(posted));
                            if(LOG)
                               fprintf(log, "\n\ncurl_easy_perform() failed to POST your magnet ;-( %s\n",
                                       curl_easy_strerror(posted));
                        }
                        else
                        {
                            if(DEBUG || LOG)
                            {
                                printf("got back HTTP responce code %d (200=OK, anything else is bad) \n", httpCode);
                                fprintf(log,"%s\nMagnet added: %s\n", currentDateTime().c_str(), XTSI.c_str());
                            }
                        }
                        if(DEBUG)
                             Sleep(321);
                    }
                    curl_easy_cleanup(curl);
                } // if(curl)
                else
                {
                    if(DEBUG || LOG)
                        fprintf(log,"%s ERR: missing curl4nas!!\n", currentDateTime().c_str());
                }
        }
            // extract filename (if we have one) or the hash
            // create $filename.torrent

            // make http request for this hash
            //  - if we could make a DHT/peer request we should
            // wait for reply
            // write .torrent to file

            //fclose(stdout); //when did we open stdout?
        } // if(argv[1])
        else
        {
            printf("Hello! Maybe you want to give me a magnet? (or I should write a config editor GUI)\n");
            fprintf(stdout,"looking for version\n");
            system("pause");
        }
    } // if(isatty(STDIN_FILENO))

    if(DEBUG)
    {
        Sleep(2579);
        system("pause");
    }

    if(DEBUG || LOG)
       fprintf(log,"%s ===Log end main()===\n", currentDateTime().c_str());
    return 0;
}
