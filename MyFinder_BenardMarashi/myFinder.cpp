#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string>
#include <iostream>
#include <dirent.h>
#include <cstring>
#include <algorithm>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

using namespace std;

/*
*
*	Funktionsdeklerationen
*
*/
int search(string path, string filename, bool recursive, bool ignoreCase);
int checkFileName(string nameSearched, string fileName, bool ignoreCase);
int isDir(string path);
void no_zombie(int signr);

int main(int argc, char *argv[])
{
	
	bool recursive = false;
	bool ignoreCase = false;

	//weniger als drei Argumente ohne Flags nicht ausreichend
	if (argc < 3)
	{
		cout << "Too few arguments.\nUsage: ./myfind [-R] [-i] <searchpath> <filename> [filename n]" << endl;
		return 1;
	}

	//die Flags -R und -i suchen und die dazugehörigen boolschen Werte auf true setzen
	int argument;
	while ((argument = getopt(argc, argv, "Ri")) != EOF)
	{
		switch (argument)
		{
		case 'R':
			recursive = true;
			break;
		case 'i':
			ignoreCase = true;
			break;
		}
	}

	//weniger als fünf Argumente mit beiden Flags nicht ausreichend
	if (recursive && ignoreCase && argc < 5)
	{
		cout << "Too few arguments.\nUsage: ./myfind [-R] [-i] <searchpath> <filename> [filename n]" << endl;
		return 1;
	}
	//weniger als vier Argumente mit einem Flag nicht ausreichend
	else if ((recursive || ignoreCase) && argc < 4)
	{
		cout << "Too few arguments.\nUsage: ./myfind [-R] [-i] <searchpath> <filename> [filename n]" << endl;
		return 1;
	}

	cout << "Start forking..." << endl;
	bool ignorePath = true;
	string searchPath;
	for (int n = 1; n < argc; ++n)
	{
		//die Flags ignorieren (fangen mit - an)
		if (argv[n][0] == '-')
		{
			continue;
		}

		//das erste Argument das kein Flag ist muss der Path sein
		// -> das erste Argument als Path speichern und alle nachfolgenden als files
		if (ignorePath)
		{
			ignorePath = false;
			searchPath = argv[n];
			continue;
		}

		//Forks erstellen
		int processID = fork();

		//Falls Kindprozess
		if (processID == 0)
		{
			//Suche
			cout << "Child: " << argv[n] << endl;
			search(searchPath, argv[n], recursive, ignoreCase);
			return 0;
		}
		//Falls kein Kindprozess
		else
		{
			
			//Zombies vermeiden
			(void) signal (SIGCHLD, no_zombie);

		}
	}

	while (wait(NULL) != -1 || errno != ECHILD);

	return 0;
}

/*
*
*	Funktionsimplementationen
*
*/

//Suche
int search(string path, string fileSearched, bool recursive, bool ignoreCase)
{
	pid_t pid = getpid();
	DIR *dirp;
	char actualPath[PATH_MAX + 1];
	struct dirent *direntp;

	//"Parent" directory öffen
	if ((dirp = opendir(path.c_str())) == NULL)
	{
		cerr << "Failed to open directory";
		return EXIT_FAILURE;
	}

	//Alle Objekte im Directory probieren
	while ((direntp = readdir(dirp)) != NULL)
	{
		//Derzeitiges Objekt im Directory [kann alles sein, nicht nur files]
		string currentObject = direntp->d_name;
		if (currentObject == "." || currentObject == "..")
		{
			continue;
		}

		//Falls das File gefunden wurde, wird dies ausgegeben
		if (checkFileName(fileSearched, currentObject, ignoreCase))
		{
			cout << pid << ": " << fileSearched << ": " << realpath(path.c_str(), actualPath) <<"/"<<fileSearched<< endl;
		}
		//Andernfalls, falls rekursiv, neuen Funktionsaufruf in einem unterliegenden Directory machen
		else
		{
			string newPath = path + "/" + currentObject;
			//Da currentObject alle Objekte im derzeitgen Directory sein kann wird durch isDir überprüft ob es ein Directory ist, welches dann durchsucht wird
			if (isDir(newPath) && recursive)
			{
				search(newPath, fileSearched, recursive, ignoreCase);
			}
		}
	}

	//"Parent" directory schließen
	while ((closedir(dirp) == -1) && (errno == EINTR));
	
	return 1;
}

//Name vergleichen (je nach ignoreCase flag)
int checkFileName(string nameSearched, string fileName, bool ignoreCase)
{
	//jeden char einzeln auf lowercase setzen
	if (ignoreCase)
	{
		for_each(nameSearched.begin(), nameSearched.end(), [](char &c)
				 { c = ::tolower(c); });
		for_each(fileName.begin(), fileName.end(), [](char &c)
				 { c = ::tolower(c); });
	}
	return nameSearched == fileName;
}

//Überprüfen ob der path ein directory ist
int isDir(string path)
{
	struct stat statbuf;
	//stat funktion lädt Informationen in die statbuf Variable
	if (stat(path.c_str(), &statbuf) != 0)
	{
		return 0;
	}
	//S_ISDIR macro überprüft ob st_mode der statbuf Variable ein directory ist
	return S_ISDIR(statbuf.st_mode);
}

//No Zombie Code aus den Folien
void no_zombie(int signr)
{
	pid_t pid;
	int ret;
	while ((pid = waitpid(-1, &ret, WNOHANG)) > 0)
	{
		cout << "Child-process with pid " << pid << " stopped." << endl;
	}
	return;
}

