//-----------------------------------------------------------------------  
//! \brief Determine the base path of the repository when a log is produced.  
//! See GPS Reference Time Status.  
//-----------------------------------------------------------------------  
std::filesystem::path GetRepoBasePath(int test_argc_, char** test_argv_, int offset_ = 0)  
{  
  if (test_argc_ > 2) { throw std::invalid_argument("Error: Invalid number of arguments provided."); }  
  else if (test_argc_ == 2)  
  {  
      try  
      {  
          return std::filesystem::path(test_argv_[1]);  
      }  
      catch (const std::exception& e)  
      {  
          std::cerr << "Error: Invalid path provided in argument 1. Exception: " << e.what() << std::endl;  
          throw;  
      }  
  }  
  else {   
      std::filesystem::path pathSourceFile = __FILE__;  
      for (int i = 0; i < offset_ + 5; ++i)  
      {  
          pathSourceFile = pathSourceFile.parent_path();  
      }  
      return pathSourceFile;  
  }  
}
