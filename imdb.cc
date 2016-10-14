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

struct actorKey {
	const char* str;
	const void* file;
};

struct filmKey {
	const film* movie;
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
	actorKey local = *(actorKey*)a;
	string s1((char*)local.file + *(int*)b);
	string s2(local.str);
	//cout << s1 << " =? " << s2 << endl;	
	if(s2 > s1) return 1;
	if(s2 < s1) return -1;
	return 0;
}

int cmpFilms(const void * a, const void * b) {
	filmKey local = *(filmKey*)a;
	string s1((char*)local.file + *(int*)b);
	film f2;
	film f1 = *local.movie;
	f2.title = s1;
	f2.year = 1900 + *(char*)((char*)local.file + *(int*)b + s1.size() + 1);
	if (f1 == f2) return 0;
	if (f1 < f2) return -1;
	return 1;
}

bool imdb::getCredits(const string& player, vector<film>& films) const 
{
	size_t num = *(int*)actorFile;
	actorKey k;
	k.str = player.c_str();
	k.file = actorFile;
	int* location = (int*)bsearch(&k, (char*)actorFile + sizeof(int), num, sizeof(int), cmpActors); // raw ადგილი, offset ინახება (int)
	if(location == NULL) return false;

	void* playerLocation = (char*)actorFile + *(int*)location; // რეალური ადგილი, აქედან იწყება მსახიობის memory chunk
	string name((char*)playerLocation);

	int offset = 1;
	if(name.size() % 2 == 0) offset *= 2;

	int arrayOffset = name.size() + offset + sizeof(short);
	if (arrayOffset % 4 != 0) {
		int tmp = arrayOffset / 4;
		arrayOffset = 4 * (tmp + 1);
	}
	short s = *(short*)((char*)playerLocation + name.size() + offset); // რამდენ ფილმშია
	cout << "has starred in " << s << " films." << endl;

	for(int i = 0; i < s; i++) 
	{
		int start = *(int*)((char*)playerLocation + arrayOffset + i * sizeof(int));
		string movie = ((char*)movieFile + start);
		film f;
		f.title = movie;
		f.year = 1900 + *(char*)((char*)movieFile + start + movie.size() + 1);
		films.push_back(f);
	}
	return true;
}

bool imdb::getCast(const film& movie, vector<string>& players) const
{
	size_t num = *(int*)movieFile;
	filmKey k;
	k.movie = &movie;
	k.file = movieFile;
	int* location = (int*)bsearch(&k, (char*)movieFile + sizeof(int), num, sizeof(int), cmpFilms);
	if(location == NULL) return false;

	void* filmLocation = (char*)movieFile + *(int*)location; // აქედან იწყება ფილმის memory chunk
	string name((char*)filmLocation);

	int offset = 1;
	if(name.size() % 2 == 0) offset = 0;
	short s = *(short*)((char*)filmLocation + name.size() + 2 + offset);

	int arrayOffset = name.size() + 2 + offset + sizeof(short);
	if (arrayOffset % 4 != 0) {
		int tmp = arrayOffset / 4;
		arrayOffset = 4 * (tmp + 1);
	}

	for (int i = 0; i < s; i++)
	{
		int start = *(int*)((char*)filmLocation + arrayOffset + i * sizeof(int));
		string player = ((char*)actorFile + start);
		players.push_back(player);
	}
	return true;
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
