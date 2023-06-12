#include <ios>
#include <fstream>

namespace VC_DUMP_METADATA
{
	std::fstream pMetaDataFile;

	bool init(std::string sOutputFileName)
	{
        std::ios_base::openmode mode = std::ios_base::out;
		bool is_open = pMetaDataFile.is_open();
        if(true == is_open)
             return false;

		pMetaDataFile.open(sOutputFileName.c_str(), mode);

        is_open = pMetaDataFile.is_open();
        if(false == is_open)
			return false;

		return true;
	}
	std::fstream& get_file()
	{
		return pMetaDataFile;
	}
	void close()
	{
		pMetaDataFile.close();
	}
	void write_tag_start(std::string str)
	{
	}
	void write_tag_end(std::string str)
	{
	}
	void write_string(std::string str)
	{
	}
	void write_int(int data)
	{
	}
	void write_float(float data)
	{
	}
	void write_double(double data)
	{
	}
	void write_bool(bool data)
	{
	}

	bool write_Dtk_Body()
	{
		return true;
	}
}