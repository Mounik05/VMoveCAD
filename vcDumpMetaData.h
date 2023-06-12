#ifndef _VC_DUMP_METADATA_
#define _VC_DUMP_METADATA_
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
	/*std::fstream& get_file()
	{
		return pMetaDataFile;
	}*/
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
		pMetaDataFile.write(str.c_str(),str.length());
	}
	void write_int(int data)
	{
		pMetaDataFile<<data;
	}
	void write_float(float data)
	{
		pMetaDataFile<<data;
	}
	void write_double(double data)
	{
		pMetaDataFile<<data;
	}
	void write_bool(bool data)
	{
	}

	bool write_shell(const Dtk_ShellPtr& inShell)
	{
		Dtk_Size_t numFace, i;
		Dtk_bool orientation;

		//fprintf(F,"<Dtk_ShellPtr>");
		write_string("Shell\n");

		//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_info(F, inShell->get_info());

		//fprintf(F,"<Id>%d</Id>",inShell->GetID());
		write_string("Id : ");
		write_int(inShell->GetID());
		write_string("\n");

		numFace = inShell->GetNumFaces();
		if(numFace == 0)
		{
			//fprintf(F,"</Dtk_ShellPtr>");
			write_string("This shell has no faces\n");
			//return dtkTopologyShellHasNoFaces;
			return true;
		}
		for(i = 0; i < numFace; i++)
		{
			Dtk_FacePtr face;
			inShell->GetFace(i, face, orientation);
			//fprintf(F, "<orientation>%d</orientation>", orientation);
			//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_FacePtr(F, face);

		}
		//fprintf(F,"</Dtk_ShellPtr>");
		return true;
	}
	bool write_volume(const Dtk_VolumePtr& inVol)
	{
		Dtk_Size_t numShell, i;

		//fprintf(F,"<Dtk_VolumePtr>");
		write_string("Volume\n");

		//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_info(F, inVol->get_info());

		//fprintf(F,"<Id>%d</Id>",inVol->GetID());
		write_string("Id : ");
		write_int(inVol->GetID());
		write_string("\n");

		numShell = inVol->GetNumShells();
		for(i = 0; i < numShell; i++)
		{
			Dtk_ShellPtr shell;
			inVol->GetShell(i, shell);
			//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_ShellPtr(F, shell);
			write_shell(shell);

		}
		//fprintf(F,"</Dtk_VolumePtr>");
		return true;
	}
	
	bool write_lump(const Dtk_LumpPtr& inLump)
	{
		Dtk_Size_t numVolume, i;

		//fprintf(F,"<Dtk_LumpPtr>");
		write_string("Lump\n");
		
		//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_info(F, inLump->get_info());
		
		//fprintf(F,"<Id>%d</Id>",inLump->GetID());
		write_string("Id :");
		write_int(inLump->GetID());
		write_string("\n");

		numVolume = inLump->GetNumVolumes();
		for(i = 0; i < numVolume; i++)
		{
			Dtk_VolumePtr volume;
			inLump->GetVolume(i, volume);
			//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_VolumePtr(F, volume);
			write_volume(volume);

		}
		//fprintf(F,"</Dtk_LumpPtr>");

		return true;
	}

	bool write_Dtk_Body(const Dtk_BodyPtr& inBody)
	{
	Dtk_Size_t numLump, i, j;

	//fprintf(F,"<Dtk_BodyPtr>");
	write_string("\n\nBody\n");
	//fprintf(F,"<Status>%d</Status>",inBody->GetBodyStatus());
	/*write_string("Status\n");
	write_int(inBody->GetBodyStatus());
	write_string("\n");*/

	//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_info(F, inBody->get_info());
	//fprintf(F,"<Id>%d</Id>",inBody->GetID());
	write_string("Id : ");
	write_int(inBody->GetID());
	write_string("\n");
	numLump = inBody->GetNumLumps();
	for(i = 0; i < numLump; i++)
	{
		Dtk_LumpPtr lump;
		inBody->GetLump(i, lump);
		//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_LumpPtr(F, lump);
		write_lump(lump);

	}
	Dtk_ShellPtr myshell;
	Dtk_Size_t m,NumOpenshell = inBody->GetNumOpenShells();

	for(m = 0 ; m<NumOpenshell ; m++ )
	{
		inBody->GetOpenShell(m,myshell);
		if(myshell.IsNotNULL())
		{
            Dtk_Size_t NumFaces = myshell->GetNumFaces();
            for (i=0;i<NumFaces;i++)
            {
                Dtk_FacePtr FacePtr;
                Dtk_bool Orientation;
                myshell->GetFace(i,FacePtr, Orientation);
                //Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_FacePtr(F, FacePtr);
            }
			Dtk_Size_t nbWires=myshell->GetNumWireSet();
			if(nbWires != 0)
			{
				//fprintf(F,"<Wireframe>");
				for(i=0;i<nbWires;i++)
				{
					Dtk_tab<Dtk_EntityPtr> wireSet;
					myshell->GetWireSet(i,wireSet);
					for (j=0;j<wireSet.size();j++)
					{
						if(wireSet[j]->get_type_detk() != DTK_TYPE_POINT)
						{
                            //Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_CurvePtr( F, Dtk_CurvePtr::DtkDynamicCast( wireSet[j] ) );
						}
						else
						{
							//Dtk_DumpXMLNamespace::Dtk_DumpXml_Dtk_PointPtr( F, Dtk_PointPtr::DtkDynamicCast( wireSet[j] ) );
						}
					}

				}
				//fprintf(F,"</Wireframe>");
			}
		}
	}

	//fprintf(F,"</Dtk_BodyPtr>");
	return true;
	}
}
#endif