
To get this to compile in Code::Blocks 13:12 on windows with g++
-----------------------------------------------------------------

In Code::Blocks:

	* Settings > Compiler... 
		* |Linker Settings| > [Add] > magnet2torrentfile\lib\libcurldll.a
		* |Search directories| > |Compiler| > [Add] > magnet2torrentfile\lib
		
		I also set
		* |Search directories| > |Linker| > [Add] > magnet2torrentfile\lib
		
It would be nice to also compile libcurl from source so that we could strip
it down to just what we need, and end up with a single binary without having
to have external .dll files.

