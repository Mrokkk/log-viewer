#pragma once

#include "core/buffer.hpp"
#include "core/fwd.hpp"
#include "core/window_node.hpp"
#include "utils/immobile.hpp"
#include "utils/trie.hpp"

namespace core
{

struct MainView : utils::Immobile
{
    MainView();
    ~MainView();

    enum class Parent
    {
        root,
        currentWindow,
    };

    constexpr auto& root()       { return mRoot; }
    constexpr auto& root() const { return mRoot; }

    constexpr auto currentWindowNode()       { return mCurrentWindowNode; }
    constexpr auto currentWindowNode() const { return mCurrentWindowNode; }

    constexpr int activeTabline() const { return mActiveTabline; }

    Buffer* currentBuffer() const;
    bool isCurrentWindowLoaded() const;
    const char* activeFileName() const;

    void initializeInputMapping(Context& context);
    void reloadAll(Context& context);
    void resize(int width, int height, Context& context);
    WindowNode& createWindow(std::string name, Parent parent, Context& context);
    void bufferLoaded(TimeOrError result, WindowNode& node, Context& context);
    void escape();
    void quitCurrentWindow(Context& context);
    void scrollTo(size_t lineNumber, Context& context);
    void searchForward(std::string pattern, Context& context);
    void searchBackward(std::string pattern, Context& context);
    void highlight(std::string pattern, std::string colorString, Context& context);

private:
    struct Impl;
    struct Pattern;

    WindowNode           mRoot;
    WindowNode*          mCurrentWindowNode;
    size_t               mWidth;
    size_t               mHeight;
    int                  mActiveTabline;
    SearchDirection      mSearchMode;
    std::string          mSearchPattern;
    utils::Trie<Pattern> mTrie;
};

}  // namespace core
