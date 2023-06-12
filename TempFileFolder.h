/* ==========================================================================
 *       Filename:  Vct/Util/TempFolder.h
 *        Company:  Visual Collaboration Technologies Inc.
 * ======================================================================== */

/**
 * \file
 * @brief Defines the TempFolder class and its methods
 *
 */

#ifndef Vct__Util__TempFolder_h
#define Vct__Util__TempFolder_h

#include <string>
#include <set>
#include <iterator>
#include <ctime>
#include <map>
//#include <process.h>
//#include <boost/predef.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

namespace Vct {
namespace Util {

    namespace fs = boost::filesystem;
    /**
     */
    class TempFileFolder
    {
    public:     // structors
        /**
         * Refer the strftime documentation for the time_format.  Enhance timeFmtToRegex to handles
         * all the cases.
         */
        TempFileFolder(const std::string& folder_format, const char* time_format = "%Y-%m-%d-%H-%M-%S") 
                : mFolderFormat(folder_format), mTimeFormat(time_format), mPath(), mFilesToRetain() 
        {
        }

        ~TempFileFolder() 
        {
        }

    public:     // methods
        /**
         */
        bool empty()
        {
            return mPath.empty();
        }

        /**
         */
        const fs::path& path() 
        {
            return mPath;
        }

        /**
         */
        fs::path parentPath() 
        {
            if(empty())
                return defaultParentPath();
            else
                return mPath.parent_path();
        }

        /**
         */
        static fs::path defaultParentPath() 
        {
        	std::string parent_path_str;
        	if(getenv("VCOLLAB_TEMP_PATH"))
        		parent_path_str = getenv("VCOLLAB_TEMP_PATH");

            if(parent_path_str.empty())
            {
#if defined(BOOST_OS_WINDOWS)
                parent_path_str = getenv("TEMP");
#else
                parent_path_str = "/tmp";
#endif
            }

            if(parent_path_str.empty())
                parent_path_str = fs::current_path().string();

            return fs::path(parent_path_str);
        }

        /**
         */
        void create()
        {
            create(defaultParentPath());
        }

        /**
         */
        void create(const fs::path& parent_path)
        {
            if(!empty())
                return;

            time_t curr_time;
            time(&curr_time);

            struct tm *timeinfo;
            timeinfo = localtime(&curr_time);

            char time_str[81];
            time_str[80] = '\0';
            strftime(time_str, 80, mTimeFormat.c_str(), timeinfo);

            std::string folder_name = mFolderFormat;
            boost::replace_all(folder_name, "%time%", time_str);
#if defined(BOOST_OS_WINDOWS)
            boost::replace_all(folder_name, "%pid%", boost::lexical_cast<std::string>(GetCurrentProcessId()));
#else
            boost::replace_all(folder_name, "%pid%", boost::lexical_cast<std::string>(getpid()));
#endif
           setPathAndCreateFolder(folder_name, parent_path);
        }
        /**
         */
        void retainFile(const std::string& file_name)
        {
            if(empty())
                return;

            mFilesToRetain.insert((mPath/file_name).string());
        }

        /**
         */
        static std::string timeFmtToRegex(const std::string& fmt)
        {
            std::string re(fmt);
            boost::replace_all(re, "%y", "\\d{2}");
            boost::replace_all(re, "%Y", "\\d{1,4}");
            boost::replace_all(re, "%m", "\\d{2}");
            boost::replace_all(re, "%d", "\\d{2}");
            boost::replace_all(re, "%H", "\\d{2}");
            boost::replace_all(re, "%M", "\\d{2}");
            boost::replace_all(re, "%S", "\\d{2}");
            
            if(re.find("%") != std::string::npos)
                throw std::runtime_error("Incomplete regex implemention for the time format");
            return re;
        }

        /**
         */
        void clearOldFolders(const fs::path& parentPath, unsigned int num_to_retain, unsigned int days_to_elapse)
        {
            if(empty())
                return;

            std::string folder_name = mFolderFormat;
            boost::replace_all(folder_name, "%time%", timeFmtToRegex(mTimeFormat));
            boost::replace_all(folder_name, "%pid%", "\\d{1,20}");
            const boost::regex re(folder_name);

            std::map<std::time_t, std::string, std::greater<time_t> > sorted_folders;
            for(fs::directory_iterator itr(mPath.parent_path()), none; itr != none; ++ itr)
            {
                const fs::path& fp = itr->path();
                if(!fs::is_directory(fp))
                    continue;

                const fs::path& fn = fp.filename();
                if(boost::regex_match(fn.string(), re))
                    sorted_folders[fs::last_write_time(fp)] = fp.string();
            }

            time_t curr_time = time(NULL);
            std::map<std::time_t, std::string, std::greater<time_t> >::iterator f_itr = sorted_folders.begin();
            for(unsigned int num = 0; f_itr != sorted_folders.end(); ++ f_itr, ++ num)
            {
                if(num < num_to_retain)
                    continue;
                if((curr_time - f_itr->first) >= (days_to_elapse * 86400 /* number of seconds per days */))
                    fs::remove_all(f_itr->second);
            }
        }

        /**
         */
        void clearOldFolders(unsigned int num_to_retain, unsigned int days_to_elapse)
        {
            clearOldFolders(parentPath(), num_to_retain, days_to_elapse);
        }

        /**
         */
        void clearTemporaryFiles()
        {
            if(empty())
                return;

            for(fs::directory_iterator itr(mPath), none; itr != none; ++ itr)
            {
                if(mFilesToRetain.find(itr->path().string()) == mFilesToRetain.end())
                    fs::remove_all(itr->path());
            }
        }

    private:    // methods
        /**
         */
        void setPathAndCreateFolder(const std::string& folder_name, const fs::path& parent_path)
        {
            if(!mPath.empty())  // folder exists
                return;

            mPath = parent_path / folder_name;
            if(!mPath.empty())
                fs::create_directory(mPath);
        }

        /**
         */
        void setPathAndCreateFolder(const std::string& folder_name)
        {
            setPathAndCreateFolder(folder_name, defaultParentPath());
        }


    private:    // attributes
        ///
        std::string mFolderFormat;

        ///
        std::string mTimeFormat;

        ///
        fs::path mPath;

        ///
        std::set<std::string> mFilesToRetain;
    };

} // namespace Util
} // namespace Vct

#endif // Vct__Util__TempFolder_h
