#pragma once

#include "store-api.hh"
#include <future>
#include <limits>

namespace nix {

    struct SubstituterSet {
        struct SubstituterResult {
            // FIXME: there's probably a better way to set this and the error messages
            bool valid = false;
            std::optional<Error> error;
            // FIXME: is this the correct name?
            std::shared_ptr<const ValidPathInfo> pathInfo;
            std::optional<StorePath> subPath;
        };

        struct SubstituterInfo {
            ref<Store> store;
            std::optional<SubstituterResult> result;
            std::future<SubstituterResult> future;
        };

        /* Content address for recomputing store path */
        std::optional<ContentAddress> contentAddress;

        /* The store path that should be realised through a substitute. */
        std::optional<StorePath> storePath;

        // The store associated with the current job?
        // FIXME: no bare pointers
        Store* baseStore = nullptr;

        // Saved list of substituters and the query results
        std::vector<SubstituterInfo> subResults;

        // Reference to the substituter in use.
        // FIXME: this really shouldn't be an index
        size_t currentSubstituter = std::numeric_limits<size_t>::max();

    public:
        SubstituterSet() = default;

        SubstituterSet(std::list<ref<Store>> substituters, std::optional<ContentAddress> ca,
                       std::optional<StorePath> storePath, Store* baseStore);

        // Perform initialization steps to query the substituters.
        void init();

        size_t remaining() const;

        // Returns the current substituter.
        // FIXME: safety
        ref<Store> getSubstituter();

        std::shared_ptr<const ValidPathInfo> getInfo() const;
        std::optional<StorePath> getSubPath() const;

        // Invalidates the current substituter and returns the next valid one.
        // To be used when the current substituter fails.
        std::optional<ref<Store>> nextSubstituter();
    };
}
