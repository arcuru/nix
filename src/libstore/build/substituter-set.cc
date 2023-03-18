#include "substituter-set.hh"
#include "store-api.hh"

namespace nix {

    SubstituterSet::SubstituterSet(std::list<ref<Store>> substituters, std::optional<ContentAddress> contentAddress,
                                   std::optional<StorePath> storePath, Store *baseStore) : contentAddress(
            contentAddress), storePath(storePath),
            baseStore(baseStore) {
        for (const auto &x: substituters) {
            subResults.push_back({.store = x});
        }
    }

    size_t SubstituterSet::remaining() const {
        return subResults.size();
    }

    // FIXME: Does this behave correctly with a missing result?
    // I think we'll need these things to return nothing on an empty list
    ref<Store> SubstituterSet::getSubstituter() {
        return subResults[currentSubstituter].store;
    }

    std::shared_ptr<const ValidPathInfo>
    SubstituterSet::getInfo() const {
        return subResults[currentSubstituter].result->pathInfo;
    }

    std::optional<StorePath>
    SubstituterSet::getSubPath() const {
        return subResults[currentSubstituter].result->subPath;
    }

    // FIXME: maybe return Ok() or Errors here? Then we use getSubstituter to access, and can log the errors better
    std::optional<ref<Store>> SubstituterSet::nextSubstituter() {
        if (subResults.empty()) {
            return {};
        }
        // What if results.size == 0?
        if (currentSubstituter >= subResults.size()) {
            // What if we continue calling this?
            currentSubstituter = 0;
        } else {
            currentSubstituter++;
            if (currentSubstituter == subResults.size()) {
                // none left
                return {};
            }
        }
        if (!subResults[currentSubstituter].result) {
            // Wait for the future if necessary
            // FIXME: choose first available substituter with matching priority
            if (subResults[currentSubstituter].future.valid()) {
                subResults[currentSubstituter].result = subResults[currentSubstituter].future.get();
            } else {
                // Future was never initialized
                // FIXME: maybe call init()? risk is some bug causing infinite looping
                assert(false);
            }
        }
        if (!subResults[currentSubstituter].result->valid) {
            return nextSubstituter();
        }
        return getSubstituter();
    }

    // Checks whether a substituter may to have the store path.
    // This function queries a single substituter and returns the result
    // FIXME: intended to be thread-safe, need to test
    // This is a non-member function because it's called async
    SubstituterSet::SubstituterResult substituterValidate(ref<Store> substituter, std::optional<ContentAddress> contentAddress, Store* baseStore, std::optional<StorePath> storePath) {
        // Explicitly mark the result as false until we finish all the checks
        auto result = SubstituterSet::SubstituterResult{.valid = false};

        if (contentAddress) {
            result.subPath = substituter->makeFixedOutputPathFromCA(storePath->name(), *contentAddress);
            if (substituter->storeDir == baseStore->storeDir)
                assert(result.subPath == storePath);
        } else if (substituter->storeDir != baseStore->storeDir) {
            result.valid = false;
            return result;
        }

        try {
            result.pathInfo = substituter->queryPathInfo(result.subPath ? *result.subPath : *storePath);
        } catch (const InvalidPath &e) {
            result.error = e;
            return result;
        } catch (const SubstituterDisabled &e) {
            result.error = e;
            return result;
        } catch (Error &e) {
            // FIXME: Handle this error message better?
            // Or combine with the others?
            logError(e.info());
            result.error = e;
            return result;
        }

        if (result.pathInfo->path != storePath) {
            if (result.pathInfo->isContentAddressed(*substituter) && result.pathInfo->references.empty()) {
                auto info2 = std::make_shared<ValidPathInfo>(*result.pathInfo);
                info2->path = *storePath;
                result.pathInfo = info2;
            } else {
                printError("asked '%s' for '%s' but got '%s'",
                           substituter->getUri(), baseStore->printStorePath(*storePath),
                           substituter->printStorePath(result.pathInfo->path));
                return result;
            }
        }

        /* Bail out early if this substituter lacks a valid
           signature. LocalStore::addToStore() also checks for this, but
           only after we've downloaded the path. */
        if (!substituter->isTrusted && baseStore->pathInfoIsUntrusted(*result.pathInfo)) {
            warn("ignoring substitute for '%s' from '%s', as it's not signed by any of the keys in 'trusted-public-keys'",
                 baseStore->printStorePath(*storePath), substituter->getUri());
            return result;
        }

        result.valid = true;
        return result;
    }

    void SubstituterSet::init() {
        for (auto &sub: subResults) {
            if (!sub.result && !sub.future.valid()) {
                sub.future = std::async(std::launch::async, substituterValidate, sub.store, contentAddress, baseStore, storePath);
            }
        }
    }

}
