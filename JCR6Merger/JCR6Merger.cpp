// License:
// 
// JCR6 Merger
// Merges JCR6 files into one
// 
// 
// 
// 	(c) Jeroen P. Broks, 2023, 2024
// 
// 		This program is free software: you can redistribute it and/or modify
// 		it under the terms of the GNU General Public License as published by
// 		the Free Software Foundation, either version 3 of the License, or
// 		(at your option) any later version.
// 
// 		This program is distributed in the hope that it will be useful,
// 		but WITHOUT ANY WARRANTY; without even the implied warranty of
// 		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// 		GNU General Public License for more details.
// 		You should have received a copy of the GNU General Public License
// 		along with this program.  If not, see <http://www.gnu.org/licenses/>.
// 
// 	Please note that some references to data like pictures or audio, do not automatically
// 	fall under this licenses. Mostly this is noted in the respective files.
// 
// Version: 24.10.24 I
// End License

#include <string>

#include <SlyvQCol.hpp>
#include <SlyvConInput.hpp>
#include <SlyvTime.hpp>

#include <JCR6_Write.hpp>
#include <JCR6_JQL.hpp>
#include <JCR6_QuakePAK.hpp>
#include <JCR6_zlib.hpp>
#include <SlyvGINIE.hpp>
#include <SlyvAsk.hpp>

using namespace std;
using namespace Slyvina;
using namespace Units;
using namespace JCR6;

#define var auto

void Process(string a);

int main(int argc, char** args) {
	init_zlib(); //JCR6_zlib.Init();
	// JCR6_lzma.Init(); // lzma not (yet) available.
	//JCR6_JXSDA.Init();
	InitQuake(); //new JCR_QuakePack();
	//new JCR_WestwoodPAK(); // Nothing but trouble
	InitWAD(); //new JCR6_WAD();
	InitJQL(); //new JCR_QuickLink();

	std::cout << endl;
	//QuickStream.Hello();
	//qstr.Hello();
	//MKL.Version("JCR6 Merger - Program.cs", "23.08.06");
	//MKL.Lic("JCR6 Merger - Program.cs", "GNU General Public License 3");

	QCol->Yellow("JCR6 Merger!\n");
	QCol->Doing("Build", __DATE__ "; " __TIME__); //QCol->Doing("Version", MKL.Newest);
	QCol->Doing("Lang", "C++");
	QCol->Doing("Coded by", "Jeroen P. Broks");
	if (argc <= 1) {
		QCol->White("Usage: ");
		QCol->Yellow("JCR6Merger ");
		QCol->Cyan("<project script>\n");
		QCol->Reset();
		return 0;
	}

	//foreach(var a in args) Process(a);
	for (int i = 1; i < argc; ++i) Process(args[i]);
	QCol->Reset();
}

vector<string> LegeVector{};
vector<string>* _AskList(GINIE Prj, string Cat, string Var, string Question) {
	return Yes(Prj, Cat, "GOFOLIST_" + Var, "Go for \"" + Question + "\"") ? AskList(Prj, Cat, Var, Question) : &LegeVector;
}

void Process(string a) {
	//var DoneFile = new Dictionary<string, bool>();
	map<string, bool> DoneFile{};
	a = ChReplace(a,'\\', '/');
	std::cout << endl;
	QCol->Doing("Project", a);
	if (!FileExists(a)) {
		QCol->Red("HEY! ");
		QCol->Yellow("Project does not exist! Create it? ");
		//if (Console.ReadKey().Key != ConsoleKey.Y) return;
		var A{ Upper(Trim(ReadLine())) }; if (a.size() == 0 && a[0] != 'Y') return;
		SaveString(a, "[Creation]\nCreation=" + CurrentDate() + "; " + CurrentTime() + "\n");
		std::cout << endl;
	}
	var Prj{ LoadGINIE(a) }; Prj->AutoSave = a; Prj->AutoSaveHeader = "JCR6Merge\nProject file!"; //= GINIE.FromFile(a); Prj.AutoSaveSource = a;
	AskGINIE = Prj; //var Ask = new TAsk(Prj);
	var ToMerge{ AskList(Prj,"Resource", "ToMerge", "Which JCR6 files are to be merged in one?") };
	var Import{ _AskList(Prj,"Resource", "ToImport", "Which JCR6 files are to be added as IMPORT?") };
	var Require{ _AskList(Prj,"Resource", "ToRequire", "Which JCR6 files are to be added as REQUIRE?") };
	var JI{ make_shared<_JT_Dir>() }; //= new TJCRDIR();
	var JO{ CreateJCR6(Ask("Output", "Output", "What to name the output JCR file? "), Ask("Compression", "File Table", "File table compression: ", "Store")) };
	for (var m:*ToMerge) { //foreach (var m in ToMerge) {
		QCol->Doing("Patching", m);
		JI->Patch(m);
	}
	for (var m :* Import) {
		QCol->Doing("Linking IMPORT", m);
		JO->Import(m);
	}
	for (var m :* Require) {
		QCol->Doing("Linking REQUIRE", m);
		JO->Require(m);
	}
	if (Yes("Comment", "Use", "Add comment")) {
		try {
			JO->AddComment(Ask("Comment", "Caption", "Comment caption: "), FLoadString(Ask("Comment", "File", "Comment file:")));
		} catch (runtime_error ex) {
			QCol->Error("Adding \"" + Prj->Value("Comment", "File") + "\" as a comment failed!");
			QCol->Magenta(ex.what());
			QCol->Grey("\n");
		}
	}
	var Blocks{ map<string, JT_CreateBlock>() };
	var Entries{ JI->Entries()};
	for (var Entry :*Entries) {
		var EntryKey(Upper(Entry->Name()));
		var Dir{ ExtractDir(EntryKey) };
		bool ToBlock{ true };
		bool Alias{ false };
		//switch (qstr.ExtractExt(Dir)) {
		if (ExtractExt(Dir) == "JFBF") ToBlock = false;
		else if (ExtractExt(Dir) == "JPBF") ToBlock = true;
		else ToBlock = Yes("BLOCKS", Dir, "Is the directory \"" + ExtractDir(Entry->Name()) + "\" a directory for a solid block");		
		for (var AEntry : *Entries) {
			var AEntryKey{ Upper(AEntry->Name()) };
			if (AEntryKey != EntryKey && DoneFile.count(AEntryKey)) {
				if (AEntry->MainFile == Entry->MainFile && AEntry->RealSize() == Entry->RealSize() && AEntry->Offset() == Entry->Offset()) {
					Alias = true;
					QCol->Doing("Aliasing", AEntry->Name(), "\t");
					QCol->Doing("as", Entry->Name());
					JO->Alias(AEntryKey, Entry->Name());
				}
			}
		}
		if (!Alias) {
			if (ToBlock) {
				if (!Blocks.count(Dir)) {
					QCol->Doing("Creating block", Dir);
					Blocks[Dir] = JO->AddBlock(Ask("Compression", "Block", "Compression method for blocks: "));
				}
				QCol->Doing("Adding", Entry->Name(), "\t");
				var buf = JI->B(EntryKey);
				Blocks[Dir]->AddBank(buf, Entry->Name(), Entry->Author(), Entry->Notes());
				var r{ JO->LastAddedEntry };
				r->_ConfigString["From"] = Entry->MainFile;
				QCol->Magenta(TrSPrintF("to block #%d\n", Blocks[Dir]->ID())); //QCol->Magenta($"to block #{Blocks[Dir].ID} \n");
			} else {
				QCol->Doing("Adding", Entry->Name(), "\t");
				var buf{ JI->B(EntryKey) };
				JO->AddBank(buf, Entry->Name(), Ask("Compression", "Entry", "Compression method for entries: "), Entry->Author(), Entry->Notes());
				var r{ JO->LastAddedEntry };
				r->_ConfigString["From"] = Entry->MainFile;
				if (r->Storage() == "Store")
					QCol->White("Stored\n");
				else
					QCol->Green(TrSPrintF("%5.1f%%", ((double)r->CompressedSize() / (double)r->RealSize()) * 100) + " " + r->Storage() + "\n");  //QCol->Green($"{r.Ratio} ({r.Storage})\n");
			}
		}
		DoneFile[EntryKey] = true;
	}
	for (var B : Blocks) {
		QCol->Doing("Closing Block", TrSPrintF("#%03d",B.second->ID()), "\t");
		B.second->Close();
		QCol->Green(TrSPrintF("%5.1f%%", ((double)B.second->CompressedSize() / (double)B.second->Size()) * 100) + "\t"); //QCol->Green($"{B.Value.Ratio}%\t");
		QCol->Magenta("#" + B.first + "\n");
	}
	QCol->Doing("Finalizing", Prj->Value("Output", "Output"));
	JO->Close();
	QCol->Green("Ok\n\n");
	QCol->Reset();
	std::cout << endl;
}
