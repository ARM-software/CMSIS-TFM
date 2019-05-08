#-------------------------------------------------------------------------------
# Copyright (c) 2019, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

#This file adds build and install targets to build the Sphinx documentation
#for TF-M. This currently means the "User Guide". Further documentation
#builds may be added in the future.

Include(CMakeParseArguments)

#This function will find the location of tools needed to build the
#documentation. These are:
#   - Mandatory:
#        - Sphinx 1.9.0 or higher
#        - PlantUML and it's dependencies
#   - Optional
#        - LateX/PDFLateX
#
#Inputs:
#   none (some global variables might be used by FindXXX modules used. For
#         details please check the modules used.)
#Outputs:
#   SPHINX_NODOC - will be defined and set to true is any mandatory tool is
#                  missing.
#   PDFLATEX_COMPILER - path to pdflatex executable
#   SPHINX_EXECUTABLE - path to sphinx-build
#   PLANTUML_JAR_PATH - path to PlantUML

#Examples
#   SphinxFindTools()
#
function(SphinxFindTools)
	#Find Sphinx
	find_package(Sphinx)

	#Find additional needed Sphinx dependencies.
	find_package(PythonModules COMPONENTS m2r sphinx-rtd-theme sphinxcontrib.plantuml)

	#Find plantUML
	find_package(PlantUML)

	#Find tools needed for PDF generation.
	find_package(LATEX COMPONENTS PDFLATEX)
	if (LATEX_PDFLATEX_FOUND)
		set (PDFLATEX_COMPILER "${PDFLATEX_COMPILER}" PARENT_SCOPE)
	endif()

	if (SPHINX_FOUND AND PLANTUML_FOUND AND PY_M2R_FOUND
			AND PY_SPHINX-RTD-THEME_FOUND AND PY_SPHINXCONTRIB.PLANTUML)
		#Export executable locations to global scope.
		set(SPHINX_EXECUTABLE "${SPHINX_EXECUTABLE}" PARENT_SCOPE)
		set(PLANTUML_JAR_PATH "${PLANTUML_JAR_PATH}" PARENT_SCOPE)
		set(Java_JAVA_EXECUTABLE "${Java_JAVA_EXECUTABLE}" PARENT_SCOPE)
		set(SPHINX_NODOC False PARENT_SCOPE)
	else()
		message(WARNING "Some tools are missing for Sphinx document generation. Document generation target is not created.")
		set(SPHINX_NODOC True PARENT_SCOPE)
	endif()
endfunction()

#Find the needed tools.
SphinxFindTools()

#If mandatory tools are missing, skip creating document generation targets.
#This means missing documentation tools is not a critical error, and building
#TF-M is still possible.
if (NOT SPHINX_NODOC)
	#The Sphinx configuration file needs some project specific configuration.
	#Variables with SPHINXCFG_ prefix are setting values related to that.
	#This is where Sphinx output will be written
	set(SPHINXCFG_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/doc_sphinx")

	set(SPHINX_TMP_DOC_DIR "${CMAKE_CURRENT_BINARY_DIR}/doc_sphinx_in")

	set(SPHINXCFG_TEMPLATE_FILE "${TFM_ROOT_DIR}/docs/conf.py.in")
	set(SPHINXCFG_CONFIGURED_FILE "${SPHINXCFG_OUTPUT_PATH}/conf.py")


	#Version ID of TF-M.
	#TODO: this shall not be hard-coded here. We need a process to define the
	#      version number of the document (and TF-M).
	set(SPHINXCFG_TFM_VERSION "1.0.0-Beta")
	set(SPHINXCFG_TFM_VERSION_FULL "Version 1.0.0-Beta")

	#Using add_custom_command allows CMake to generate proper clean commands
	#for document generation.
	add_custom_command(OUTPUT "${SPHINX_TMP_DOC_DIR}"
		COMMAND "${CMAKE_COMMAND}" -D TFM_ROOT_DIR=${TFM_ROOT_DIR} -D DST_DIR=${SPHINX_TMP_DOC_DIR} -P "${TFM_ROOT_DIR}/cmake/SphinxCopyDoc.cmake"
		WORKING_DIRECTORY "${TFM_ROOT_DIR}"
		VERBATIM
		)

	add_custom_target(create_sphinx_input
		SOURCES "${SPHINX_TMP_DOC_DIR}"
	)

	add_custom_command(OUTPUT "${SPHINXCFG_OUTPUT_PATH}/html"
		COMMAND "${SPHINX_EXECUTABLE}" -c "${SPHINXCFG_OUTPUT_PATH}" -b html "${SPHINX_TMP_DOC_DIR}" "${SPHINXCFG_OUTPUT_PATH}/html"
		WORKING_DIRECTORY "${TFM_ROOT_DIR}"
		DEPENDS create_sphinx_input
		COMMENT "Running Sphinx to generate user guide (HTML)."
		VERBATIM
		)

	#Create build target to generate HTML documentation.
	add_custom_target(doc_userguide
		COMMENT "Generating User Guide with Sphinx..."
		#Copy document files from the top level dir to docs
		SOURCES "${SPHINXCFG_OUTPUT_PATH}/html"
		)

	#Add the HTML documentation to install content
	install(DIRECTORY ${SPHINXCFG_OUTPUT_PATH}/html DESTINATION doc/user_guide
		EXCLUDE_FROM_ALL
		COMPONENT user_guide
		PATTERN .buildinfo EXCLUDE
		)

	#If PDF documentation is being made.
	if (PDFLATEX_COMPILER)
		set(_PDF_FILE "${SPHINXCFG_OUTPUT_PATH}/latex/TF-M.pdf")

		add_custom_command(OUTPUT "${SPHINXCFG_OUTPUT_PATH}/latex"
			COMMAND "${SPHINX_EXECUTABLE}" -c "${SPHINXCFG_OUTPUT_PATH}" -b latex "${SPHINX_TMP_DOC_DIR}" "${SPHINXCFG_OUTPUT_PATH}/latex"
			WORKING_DIRECTORY "${TFM_ROOT_DIR}"
			DEPENDS create_sphinx_input
			COMMENT "Running Sphinx to generate user guide (LaTeX)."
			VERBATIM
			)

		add_custom_command(OUTPUT "${_PDF_FILE}"
			COMMAND "${CMAKE_MAKE_PROGRAM}" all-pdf
			WORKING_DIRECTORY ${SPHINXCFG_OUTPUT_PATH}/latex
			DEPENDS "${SPHINXCFG_OUTPUT_PATH}/latex"
			COMMENT "Generating PDF version of User Guide..."
			VERBATIM
			)

		#We do not use the add_custom_command trick here to get proper clean
		#command since the clean rules "added" above will remove the entire
		#doc directory, and thus clean the PDF output too.
		add_custom_target(doc_userguide_pdf
			COMMENT "Generating PDF version of TF-M User Guide..."
			SOURCES "${_PDF_FILE}"
			VERBATIM)

		#Add the pdf documentation to install content
		install(FILES "${_PDF_FILE}" DESTINATION "doc/user_guide"
			RENAME "tf-m_user_guide.pdf"
			COMPONENT user_guide
			EXCLUDE_FROM_ALL)
	else()
		message(WARNING "PDF generation tools are missing. PDF document generation target is not created.")
	endif()

	#Generate build target which installs the documentation.
	if (TARGET doc_userguide_pdf)
		add_custom_target(install_userguide
			DEPENDS doc_userguide doc_userguide_pdf
			COMMAND "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=user_guide
			-P "${CMAKE_BINARY_DIR}/cmake_install.cmake")
	else()
		add_custom_target(install_userguide
			DEPENDS doc_userguide
			COMMAND "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=user_guide
			-P "${CMAKE_BINARY_DIR}/cmake_install.cmake")
	endif()

	#Now instantiate a Sphinx configuration file from the template.
	message(STATUS "Writing Sphinx configuration...")
	configure_file(${SPHINXCFG_TEMPLATE_FILE} ${SPHINXCFG_CONFIGURED_FILE} @ONLY)
endif()
