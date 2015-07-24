macro( create_git_version_control_header )

	#---generate (when necessary) git version/branch information header KVGitInfo.h
	#
	# * if Git is not available, this must be a .tgz with an existing KVGitInfo.h in the source directory,
	#     therefore we copy the existing file
	
	find_package(Git QUIET)
	
	if(GIT_FOUND)
	
		if(EXISTS ${CMAKE_SOURCE_DIR}/.git)
		
			#--- sources are under version control
			    		
			add_custom_target(vcsinfos
				COMMAND ${CMAKE_SOURCE_DIR}/tools/gitinfo.sh ${GIT_EXECUTABLE} ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/KVGitInfo.h
				WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
			)
			 
		else()
			
			#--- sources are not under version control
			 
			if(NOT EXISTS ${CMAKE_SOURCE_DIR}/KVGitInfo.h) #--- strange: Git is not installed, and no KVGitInfo.h exists???
			
				message(FATAL_ERROR "not a git branch, and ${CMAKE_SOURCE_DIR}/KVGitInfo.h not found???")
		
			else()
		
				#--- copy existing file to build directory
				configure_file(${CMAKE_SOURCE_DIR}/KVGitInfo.h ${CMAKE_BINARY_DIR})
		
			endif()
			
		endif()
		
	else() #--- Git is not available: therefore we use the existing file (if there is one)
		
		if(NOT EXISTS ${CMAKE_SOURCE_DIR}/KVGitInfo.h) #--- strange: Git is not installed, and no KVGitInfo.h exists???
			
			message(FATAL_ERROR "git not found, and ${CMAKE_SOURCE_DIR}/KVGitInfo.h not found???")
		
		else()
		
			#--- copy existing file to build directory
			configure_file(${CMAKE_SOURCE_DIR}/KVGitInfo.h ${CMAKE_BINARY_DIR})
		
		endif()
	
	endif()

endmacro( create_git_version_control_header )
