using namespace std;
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "imdb.h"
#include <stdio.h>
#include <stdlib.h>

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";

struct key {
	const char* str;
	const void* file;
};

imdb::imdb(const string& directory)
{
	const string actorFileName = directory + "/" + kActorFileName;
	const string movieFileName = directory + "/" + kMovieFileName;
	
	actorFile = acquireFileMap(actorFileName, actorInfo);
	movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const
{
	return !( (actorInfo.fd == -1) || 
			(movieInfo.fd == -1) ); 
}

int cmpActors(const void * a, const void * b)
{
	key local = *(key*)a;
	string s1((char*)local.file + *(int*)b);
	string s2(local.str);
	// cout << s1 << " =? " << s2 << endl;	
	if(s2 > s1) return 1;
	if(s2 < s1) return -1;
	return 0;
}

bool imdb::getCredits(const string& player, vector<film>& films) const 
{
	size_t num = *(int*)actorFile;

	{
		// cout << actorNumber << endl;
		// for (size_t i = 1; i <= num; i++)
		// {
		// 	int temp = *(int*)((char*)actorFile + i*sizeof(int));	
		// 	string s((char*)actorFile + temp);
		// 	cout << s << endl;
		// 	if (s == "999") break;		
		// }
		// int* i = (int*)(char*)actorFile + sizeof(int);
	}
	
	key k;
	k.str = player.c_str();
	k.file = actorFile;
	int* location = (int*)bsearch(&k, (char*)actorFile + sizeof(int), num, sizeof(int), cmpActors); // raw ადგილი, offset ინახება (int)
	if(location == NULL) return false;

	void* playerLocation = (char*)actorFile + *(int*)location; // რეალური ადგილი, აქედან იწყება მსახიობის memory chunk
	// cout << "აააააააააააა: " << (char*)playerLocation << endl;
	string name((char*)playerLocation);
	// cout << name << endl;
	short s; // რამდენ ფილმშია
	int arrayOffset;

	if(name.size() % 2 == 0) 
	{
		arrayOffset = name.size() + 2 + sizeof(short);
		if (arrayOffset % 4 != 0) {
			int tmp = arrayOffset / 4;
			arrayOffset = 4 * (tmp + 1);
		}
		s = *(short*)((char*)playerLocation + name.size() + 2);
		cout << "has starred in " << s << " films." << endl;
	} else
	{
		arrayOffset = name.size() + 1 + sizeof(short);
		if (arrayOffset % 4 != 0) {
			int tmp = arrayOffset / 4;
			arrayOffset = 4 * (tmp + 1);
		}
		s = *(short*)((char*)playerLocation + name.size() + 1);
		cout << "has starred in " << s << " films." << endl;
	}

	int* arrayStart = (int*)((char*)playerLocation + arrayOffset);
	string movie((char*)movieFile + *arrayStart);
	cout << movie << " " << 1900 + *(char*)((char*)movieFile + *arrayStart + movie.size() + 1) << endl;
	return false;
}

bool imdb::getCast(const film& movie, vector<string>& players) const
{ 
	return false; 
}

imdb::~imdb()
{
	releaseFileMap(actorInfo);
	releaseFileMap(movieInfo);
}

// ignore everything below... it's all UNIXy stuff in place to make a file look like
// an array of bytes in RAM.. 
const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info)
{
	struct stat stats;
	stat(fileName.c_str(), &stats);
	info.fileSize = stats.st_size;
	info.fd = open(fileName.c_str(), O_RDONLY);
	return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info)
{
	if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
	if (info.fd != -1) close(info.fd);
}
