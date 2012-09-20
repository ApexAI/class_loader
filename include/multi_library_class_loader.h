/*
 * Copyright (c) 2012, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PLUGINS_MULTI_LIBRARY_CLASS_LOADER_H_DEFINED
#define PLUGINS_MULTI_LIBRARY_CLASS_LOADER_H_DEFINED

#include "class_loader.h"
#include <boost/thread.hpp>

namespace plugins
{

typedef std::string LibraryPath;
typedef std::map<LibraryPath, plugins::ClassLoader*> LibraryToClassLoaderMap;
typedef std::vector<ClassLoader*> ClassLoaderVector;

class MultiLibraryClassLoader
{
  public:
    /**
     * @brief Constructor for the class
     * @param enable_ondemand_loadunload - Flag indicates if classes are to be loaded/unloaded automatically as plugins are created and destroyed
     */
    MultiLibraryClassLoader(bool enable_ondemand_loadunload);

    /**
    * @brief Virtual destructor for class
    */
    virtual ~MultiLibraryClassLoader();

    /**
     * @brief Creates an instance of an object of given class name with ancestor class Base
     * This version does not look in a specific library for the factory, but rather the first open library that defines the classs
     * @param Base - polymorphic type indicating base class
     * @param class_name - the name of the concrete plugin class we want to instantiate
     * @return A boost::shared_ptr<Base> to newly created plugin
     */
    template <class Base>
    boost::shared_ptr<Base> createInstance(const std::string& class_name)
    {
      ClassLoaderVector active_loaders = getAllAvailableClassLoaders();
      for(unsigned int c = 0; c < active_loaders.size(); c++)
      {
        ClassLoader* current = active_loaders.at(c);
        if(current->isClassAvailable<Base>(class_name))
          return(current->createInstance<Base>(class_name));
      }

      throw(plugins::CreateClassException("MultiLibraryClassLoader: Could not create class of type " + class_name + " as no class loader exists."));
    }

    /**
     * @brief Creates an instance of an object of given class name with ancestor class Base
     * This version takes a specific library to make explicit the factory being used
     * @param Base - polymorphic type indicating base class
     * @param class_name - the name of the concrete plugin class we want to instantiate
     * @param library_path - the library from which we want to create the plugin
     * @return A boost::shared_ptr<Base> to newly created plugin
     */
    template <class Base>
    boost::shared_ptr<Base> createInstance(const std::string& class_name, const std::string& library_path)
    {
      return(getClassLoaderForLibrary(library_path)->createInstance<Base>(class_name));
    }

    /**
     * @brief Creates an instance of an object of given class name with ancestor class Base
     * This version does not look in a specific library for the factory, but rather the first open library that defines the classs
     * This version should not be used as the plugin system cannot do automated safe loading/unloadings
     * @param Base - polymorphic type indicating base class
     * @param class_name - the name of the concrete plugin class we want to instantiate
     * @return An unmanaged Base* to newly created plugin
     */
    template <class Base>
    Base* createUnmanagedInstance(const std::string& class_name)
    {
      ClassLoaderVector active_loaders = getAllAvailableClassLoaders();
      for(unsigned int c = 0; c < active_loaders.size(); c++)
      {
        ClassLoader* current = active_loaders.at(c);
        if(current->isClassAvailable<Base>(class_name))
          return(current->createUnmanagedInstance<Base>(class_name));
      }

      throw(plugins::CreateClassException("MultiLibraryClassLoader: Could not create class of type " + class_name));
    }

    /**
     * @brief Creates an instance of an object of given class name with ancestor class Base
     * This version takes a specific library to make explicit the factory being used
     * This version should not be used as the plugin system cannot do automated safe loading/unloadings
     * @param Base - polymorphic type indicating Base class
     * @param class_name - name of class for which we want to create instance
     * @param library_path - the fully qualified path to the runtime library
     */
    template <class Base>
    Base* createUnmanagedInstance(const std::string& class_name, const std::string& library_path)
    {
      return(getClassLoaderForLibrary(library_path)->createUnmanagedInstance<Base>(class_name));
    }

    /**
     * @brief Indicates if a class has been loaded and can be instantiated
     * @param Base - polymorphic type indicating Base class
     * @param class_name - name of class that is be inquired about
     * @return true if loaded, false otherwise
     */
    template <class Base>
    bool isClassAvailable(const std::string& class_name)
    {
      std::vector<std::string> available_classes = getAvailableClasses<Base>();
      return(available_classes.end() != std::find(available_classes.begin(), available_classes.end(), class_name));
    }

    /**
     * @brief Indicates if a library has been loaded into memory
     * @param library_path - The full qualified path to the runtime library
     * @return true if library is loaded, false otherwise
     */
    bool isLibraryAvailable(const std::string& library_path);

    /**
     * @brief Gets a list of all classes that are loaded by the class loader
     * @param Base - polymorphic type indicating Base class
     * @return A vector<string> of the available classes
     */
    template <class Base>
    std::vector<std::string> getAvailableClasses()
    {
      std::vector<std::string> available_classes;
      ClassLoaderVector loaders = getAllAvailableClassLoaders();
      for(unsigned int c = 0; c < loaders.size(); c++)
      {
        ClassLoader* current = loaders.at(c);
        std::vector<std::string> loader_classes = current->getAvailableClasses<Base>();
        available_classes.insert(available_classes.end(), loader_classes.begin(), loader_classes.end());
      }
      return(available_classes);
    }

    /**
     * @param Base - polymorphic type indicating Base class
     * @return A vector<string> of the available classes in the passed library
     */
    template <class Base>
    std::vector<std::string> getAvailableClassesForLibrary(const std::string& library_path)
    {
      ClassLoader* loader = getClassLoaderForLibrary(library_path);
      std::vector<std::string> available_classes;
      if(loader)
        available_classes = loader->getAvailableClasses<Base>();
      return(available_classes);
    }

    /**
     * @brief Gets a list of all libraries opened by this class loader
     @ @return A list of libraries opened by this class loader
     */
    std::vector<std::string> getRegisteredLibraries();

    /**
     * @brief Loads a library into memory for this class loader
     * @param library_path - the fully qualified path to the runtime library
     */
    void loadLibrary(const std::string& library_path);

    /**
     * @brief Unloads a library for this class loader
     * @param library_path - the fully qualified path to the runtime library
     */
    void unloadLibrary(const std::string& library_path);


  private:
    /**
     * @brief Indicates if on-demand (lazy) load/unload is enabled so libraries are loaded/unloaded automatically as needed
     */
    bool isOnDemandLoadUnloadEnabled(){return(enable_ondemand_loadunload_);}

    /**
     * @brief Gets a handle to the class loader corresponding to a specific runtime library
     * @param library_path - the library from which we want to create the plugin
     * @return A pointer to the ClassLoader*, == NULL if not found
     */
    ClassLoader* getClassLoaderForLibrary(const std::string& library_path);

    /**
     * @brief Gets all class loaders loaded within scope
     */
    ClassLoaderVector getAllAvailableClassLoaders();
    void shutdownAllClassLoaders();

  private:
    bool enable_ondemand_loadunload_;
    LibraryToClassLoaderMap active_class_loaders_;
    boost::mutex loader_mutex_;
};


}
#endif
