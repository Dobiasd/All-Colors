import fnmatch
import sys
import os


source_files = []
for root, dirnames, filenames in os.walk('src'):
    for filename in filenames:
        if fnmatch.fnmatch(filename, '*.cpp'):
            source_files.append(str(os.path.join(root, filename)))

build_dir = 'build'

# Windows
if sys.platform[:3] == 'win':
	build_dir = 'build_win'
	env = Environment(tools = ['mingw'])

	# Insert your SFML paths here.
	# env.Append(CPPPATH='D:\Tobias\Projekte\SFML\SFML-1.6_MinGW\include')
	# env.Append(LIBPATH='D:\Tobias\Projekte\SFML\SFML-1.6_MinGW\lib\mingw')


# Linux etc.
else:
	env = Environment()


VariantDir(build_dir, 'src', duplicate=0)
source_files = [s.replace('src', build_dir, 1) for s in source_files]

#env.Append(LIBS=['sfml-audio', 'sfml-graphics','sfml-window','sfml-system'])
env.Append(LIBS=['opencv_core', 'opencv_imgproc', 'opencv_highgui', 'opencv_ml', 'opencv_video', 'opencv_features2d', 'opencv_calib3d', 'opencv_objdetect', 'opencv_contrib', 'opencv_legacy', 'opencv_flann'])
env.Append(CXXFLAGS='-std=c++11 -Wall -Wextra -pedantic -Werror')
env.Program(target='release/AllColors', source=source_files)