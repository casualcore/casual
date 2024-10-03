#include "common/transaction/context.h"

#include <filesystem>

namespace casual::file::resource
{
    std::filesystem::path acquire( const common::transaction::ID& transaction, std::filesystem::path origin);

    //! commit
    void perform( const common::transaction::ID& transaction);
    //! rollback
    void restore( const common::transaction::ID& transaction);

} // casual::file::resource