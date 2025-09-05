#pragma once
#include "CText.h"
#include "crc32.h"
#include <string>
#include <unordered_map>

namespace CLEO {
class CTextManager {
  class FxtEntry {
    friend class CTextManager;
    std::string text;
    bool is_static;
    FxtEntry(const char *_text, bool _static = false);
  };

  typedef std::unordered_map<std::string, FxtEntry *,
                             decltype(&crc32FromUpcaseStdString)>
      fxt_map_type;
  typedef fxt_map_type::iterator fxt_iterator;
  typedef fxt_map_type::const_iterator const_fxt_iterator;
  fxt_map_type fxts;

public:
  CTextManager();
  ~CTextManager();

  void LoadFxts();
  void Clear();

  const char *Get(const char *key);
  bool AddFxt(const char *key, const char *value, bool dynamic = true);
  bool RemoveFxt(const char *key);
  // find fxt text by its key
  const char *LocateFxt(const char *key);
  // erase all fxts, added by scripts
  void ClearDynamicFxts();
  size_t ParseFxtFile(std::istream &stream, bool dynamic = false,
                      bool remove = false);
};
} // namespace CLEO
