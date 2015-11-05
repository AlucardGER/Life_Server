/*
 * PBO file extractor
 * ------------------
 * This program uses the dll to expand all files into their 
 * respective directories.
 *
 * syntax: see below
 */
/*
** extraction behaviour
**
** Fundamentally, a folder is created of the same name as the pbo in the same folder as the pbo.
**
** Arma pbo's will, in addition, create subfolders based on the detected prefix. Thus:
**
** ExtractPbo Thing
**
** OFP pbo 		thing.pbo -> thing\.....
** ARMA pbo     thing.pbo -> thing\Prefix\......
**
** Option -K is used to force OFP behaviour
**
** ExtractPbo -k Thing //Arma prefix subfolders are NOT created (but the $PREFIX% is still supplied correctly)
**
**				thing.pbo -> thing\..... (prefix is ignored)
**
**============================
** Specifying a destination
**============================
**
ExtractPbo thing  P:\
	thing.pbo ->P:\prefix\....	will create a perfect namespace based on the prefix.

	ExtractPbo thing  -k P:\
	thing.pbo ->P:\thing\prefix\....

ExtractPbo thing  P:\SomeWhere

	thing.pbo ->P:\Somewhere\prefix\...

ExtractPbo -K thing  P:\SomeWhere

	thing.pbo ->P:\Somewhere\thing\.........

	
	=============================
	Specifying a relative address
	=============================


*/
#include "stdmain.hpp"

#undef VERSION
#define VERSION 1.80

int ProcessPbo(Asciiz* filename,CALLBACK_PRINT(callback),int flags,Asciiz* FilesToExtract,Asciiz* Where);
int ProcessList(Asciiz* PboName,CALLBACK_PRINT(callback),int flags,Asciiz* FilesToExtract,Asciiz* Where);
int ProcessFile(Asciiz* PboName,CALLBACK_PRINT(callback),int flags,Asciiz* FilesToExtract,Asciiz* Where);
int ProcessFolder(Asciiz* PboName,CALLBACK_PRINT(callback),int flags,Asciiz* FilesToExtract,Asciiz* Where);

TRY_SILENT_MAIN

char *NamesOfFiles=0;
Asciiz*   option;
Asciiz*   Where="";

	int flags=DEPBO_EXTRACT_DERAP;    
	bool list_only=false;
	bool ask=true;
	int vbstype=0;

	for(;;)
	{
		if (argv < 2)
		{
syntax:
			_PRINT_TITLE;
			fprintf(stderr,"\nSyntax: %s [-options...] PboName[.pbo|.xbo|.ebo]|FolderName|Extraction.lst  [Drive:[\\Fully\\Qualified\\Destination\\Folder]]\n", exename);
			fprintf(stderr,"\nextensions are not required\n\n");
			fprintf(stderr,"\nA FullyQualifiedDestinationFolder *must* contain a drive specifier\n\n");
			fprintf(stderr,"options (case insensitive)\n\n");
			fprintf(stderr,"  -S Silent (default)\n");
			fprintf(stderr,"  -N Noisy\n");
			fprintf(stderr,"  -L list contents only (do not extract)\n");
			fprintf(stderr,"  -LB brief listing (dir style) \n");
			fprintf(stderr,"  -Y Don't prompt if overwriting\n");
			fprintf(stderr,"  -D DeRapify when appropriate (default)\n");
			fprintf(stderr,"  -R Don't DeRapify\n");
			fprintf(stderr,"  -W warnings are errors\n");
			fprintf(stderr,"  -F PboFile(s)ToExtract[,...]\n");
			fprintf(stderr,"  -P Don't pause on error\n");

			fprintf(stderr," \n*Vbs2 lite options (normally not required self detect)\n");
			fprintf(stderr,"  -V1 force vbs2 lite UK extraction\n");
			fprintf(stderr,"  -V2 force vbs2 lite US extraction\n\n");

			fprintf(stderr," *Output to pboname\\ or 'prefix'\\\n");
			fprintf(stderr,"  -K ignore prefix (ofp behaviour)\n");
			fprintf(stderr,"\nExamples\n\n");
			fprintf(stderr," extractpbo thing\n");
			fprintf(stderr,"  OFP : Extracts thing.pbo to thing\\ \n");
			fprintf(stderr,"  ARMA: Extracts thing.pbo to thing\\'prefix'\\\n");

			fprintf(stderr,"   Derapifies any mission.sqm, config.bin, or rvmat file if binarised\n");
			fprintf(stderr,"   (config.bin is renamed to config.cpp)\n\n");
			fprintf(stderr," extractpbo -R thing\n");
			fprintf(stderr,"  As above but no derapping takes place\n\n");

			fprintf(stderr," extractpbo thing Drive:\\MySafeFolder\n");
			fprintf(stderr,"  ARMA: Extracts thing.pbo to Drive:\\MySafeFolder\\'armaprefix'\\\n\n");
			fprintf(stderr," extractpbo thing P:\\\n");
			fprintf(stderr,"  Creates a correct Virtual Namespace of the pbo\n\n");
			fprintf(stderr," extractpbo -F mission.sqm thing.pbo\n");
			fprintf(stderr,"  will extract ALL occurences of mission.sqm in the pbo (and derapify)\n");
			fprintf(stderr," extractpbo -NF config.bin,motocross3.wss,*.p3d wheeled3\n");
			fprintf(stderr,"  noisy extraction of specific files (eg lists them)\n");
			return 1;
		}

		option=argc[1]; // possibly
		if (*option++!='-') break;
		argc++;
		argv--;
		while (*option)
		{
			switch(toupper(*option++))
			{
			default: goto syntax;
			case 'D': break;// now the default
			case 'A': break;// default deprecated flags|=DEPBO_EXTRACT_WRITE_PREFIX; break;
			case 'R': flags&=~DEPBO_EXTRACT_DERAP; break;  
			case 'B': flags|=(DEPBO_EXTRACT_BRIEF|DEPBO_EXTRACT_NOISY); break;
			case 'N': flags|=DEPBO_EXTRACT_NOISY; break;
			case 'S': flags&=~DEPBO_EXTRACT_NOISY; break;
			case 'Y': ask=false; break;
			case 'P': PauseOnError=false; break;
			case 'V': 

				switch(*option++)
				{
				default: goto syntax;
				case '1':	vbstype=PBOTYPE_VBS2_LITE_XBO_UK|PBOTYPE_VBS2_LITE_MISSION_UK; break;
				case '2':	vbstype=PBOTYPE_VBS2_LITE_XBO_US|PBOTYPE_VBS2_LITE_MISSION_US; break;
				}
				break;

			case 'F': 

				if (argv < 2) goto syntax;
				NamesOfFiles=argc[1];
				argv--;
				argc++;
				break;

			case 'L':flags= (DEPBO_EXTRACT_LIST_ONLY|DEPBO_EXTRACT_NOISY); ask=false; break;
			case 'K': flags |= DEPBO_EXTRACT_IGNORE_PREFIX; break;
			case 'W': flags |= DEPBO_EXTRACT_WARNINGS_ARE_ERRORS; break;
			}
		}
	}
	if (!(flags&DEPBO_EXTRACT_BRIEF)) _PRINT_TITLE;
	flags|=vbstype;
	if (argv>3) goto syntax;
	if (argv==3)		//exe pbo dest
	{
		Where=argc[2]; ask=false;
		if (strlen(Where)<3 || Where[1]!=':' || (Where[2]!='\\' && Where[2]!='/')) goto syntax;
	}
	char *PboName=argc[1];
	// first check if we are dealing with a pbo, a file list, or, a folder
	// this extension check must be done first because a folder and a folder.pbo can exist at same time

	FileString checker(PboName);
	if (checker.CompareList("*.pbo\0*.ebo\0*.xbo\0")) goto ispbo;
	//if (checker.co.CompareExtList("pbo")||checker.CompareExt("ebo")||checker.CompareExt("xbo")) goto ispbo;
	if (checker.FileExists(checker.AddExt("pbo"))) goto ispbo;
	if (checker.FileExists(checker.ReplaceExt("ebo"))) goto ispbo;
	if (checker.FileExists(checker.ReplaceExt("xbo"))) goto ispbo;


/* can only be a list or folder. this test done after file check so all is ok
*/

	if (checker.FolderExists(PboName)) 
	{
		if ((flags&DEPBO_EXTRACT_NOISY)==0) printf("processing folder(s)...\n");
	
		return ProcessFolder(PboName,(flags&DEPBO_EXTRACT_NOISY)?&CallbackPrint:0,flags,NamesOfFiles,Where);
	}

/*
** can only be a file list if anything at all
*/
	int sts= ProcessList(PboName,(flags&DEPBO_EXTRACT_NOISY)?&CallbackPrint:0,flags,NamesOfFiles,Where);
	if (sts==-1) goto syntax;
	return sts;

ispbo:
	/*
	*/
	if (ask)
	{
		/* here, we are just using the depbo internals that generate this stuff
		** to check where the ultimate output folder will be
		*/
		if (!DePbo->PboFile->MakeFilename(PboName,"pbo")) return 1;	// this strips all the microsoft shit off the pathing the pbo extension is irrelevent
		FileString   folder(DePbo->BuildDestFolder(Where));
		if (folder.FolderExists())
		{
			if (!AskUser("%s exists. Overwrite?",folder.Get())) return 1;
			printf("ok...\n");
		}
	}
	return ProcessPbo(PboName,(flags&DEPBO_EXTRACT_NOISY)?&CallbackPrint:0,flags,NamesOfFiles,Where);


CATCH_MAIN
int ProcessList(Asciiz* PboName,CALLBACK_PRINT(callback),int flags,Asciiz* FilesToExtract,Asciiz*   Where)
{
	FileString FileList;
	/* expand this to accept individual options
	*/
	if (!FileList.ReadTextFile("%s",PboName)) return -1;

	FileString Stripper;
	while (PboName=(char*)FileList.GetNextLine())// 1st call gives 1st line
	{
		CallbackPrint("\n%s->%s",Stripper.GetFilenameAndExtOnly(PboName),Where);		// allows user to hit esc key
		if (ProcessPbo(PboName,(flags&DEPBO_EXTRACT_NOISY)?&CallbackPrint:0,flags,FilesToExtract,Where)) return 1;
	}
	return 0;
}
bool IsPbo (FileString *thing)
{
	return thing->CompareExt("pbo")||thing->CompareExt("ebo")||thing->CompareExt("xbo");
}
int ProcessPbo(Asciiz* PboName,CALLBACK_PRINT(callback),int flags,Asciiz*   FilesToExtract,Asciiz*   Where)
{
int	sts=DePbo->ExtractPbo(PboName,callback,flags,FilesToExtract,Where);
	if (sts || !(flags&DEPBO_EXTRACT_BRIEF))	fprintf(stderr,"\n%s\n",DePbo->ErrorString(sts));	
	return sts?1:0;
}

//p:\test\foldertest

#include "PboUtils.h"

int ProcessFolder(Asciiz* PboName,CALLBACK_PRINT(callback),int flags,Asciiz* FilesToExtract,Asciiz* Where)
{

	FileTree ft;
	FileString FileOrFolder;


	switch(ft.OpenDir(PboName))
	{
	case FILETREE_ERROR:		return 1;
	case FILETREE_OK: 

		do
		{
			FileOrFolder.Format("%s\\%s", PboName,  ft.GetFname());
			if (ft.IsDir())
			{
				if (callback) callback("%s\n",FileOrFolder.Get());
				if (ProcessFolder(FileOrFolder.Get(),callback,flags,FilesToExtract,Where)) return 1;
			}
			else	
			{
				if (IsPbo(&FileOrFolder))
				{
					printf("%s.......",FileOrFolder.Get());
					if (ProcessPbo(FileOrFolder.Get(),callback,flags,FilesToExtract,Where)) return 1;
				}
			}
		}while (ft.NextFname());

	case FILETREE_ISEMPTY: break;
	}
	ft.CloseDir();
	return 0;
}

