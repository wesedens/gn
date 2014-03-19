gn
==

standalone mirror of the generate ninja project from chromium

gn uses the same build tools as Chromium, so you'll need to set up the build environment as described in the [Chromium instructions](http://www.chromium.org/developers).

Once this is done, you can fetch the sources by doing:

>`mkdir gn`  
>`cd gn`  
>`gclient config https://github.com/wesedens/gn.git --name=src`  
>`gclient sync`  

gn is built with ninja. Once you have the source code you can generate the ninja files by running the following command.

>`cd src`  
>`gn gen out/Debug`  

TODO:
I haven't figured out how to generate a release build or run `gn` from `gclient` yet because `gclient` runs actions in a python subprocess with the cwd above src and you can't change directories within the child process.
