
// ZeroToHero (Sample Project)

//#define HERO_USING_STD

#ifdef HERO_USING_STD
#include <iostream>
#include <string>
#include <map>
#endif


#include "hero/file.h"
#include "hero/string.h"
#include "hero/assert.h"

int main(int argc, char * argv[])
{
    PrintLn("ZeroToHero");
  
    #ifdef HERO_USING_STD
    std::cout << "Hello from the other side..." << std::endl;
    #endif

    // When executed this file will print its own name and size and
    // assert that the source code contains file.Read(data)

    String data;
    File file(__FILE__);
    file.Read(data);

    PrintLn("File: %s",__FILE__);
    PrintLn("Size: %d",data.Size);

    Assert(data.Contains("file.Read(data)"));
}