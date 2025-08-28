#include "readline.hpp"

#include <flat_set>
#include <ranges>

#include "core/input.hpp"
#include "utils/bitflag.hpp"
#include "utils/string.hpp"

namespace core
{

namespace
{

using StringSet = std::flat_set<std::string>;

struct History final : utils::Immobile
{
    struct Iterator final
    {
        explicit Iterator(utils::Strings& content);
        ~Iterator();

        bool isBeginning() const;
        operator bool() const;
        const std::string& operator*() const;
        Iterator& operator--();
        Iterator& operator++();
        void reset();

    private:
        utils::Strings&          mContent;
        utils::Strings::iterator mIterator;
    };

    History();
    ~History();

    void pushBack(const std::string& entry);
    void clear();

    utils::Strings content;
    Iterator       current;

private:
    StringSet      mContentSet;
};

History::History()
    : current(content)
{
}

History::~History() = default;

void History::pushBack(const std::string& entry)
{
    auto result = mContentSet.emplace(entry);

    if (result.second)
    {
        content.push_back(entry);
        current.reset();
    }
}

void History::clear()
{
    content.clear();
    current.reset();
}

History::Iterator::Iterator(utils::Strings& content)
    : mContent(content)
    , mIterator(mContent.end())
{
}

History::Iterator::~Iterator() = default;

bool History::Iterator::isBeginning() const
{
    return mIterator == mContent.begin();
}

History::Iterator::operator bool() const
{
    return mIterator != mContent.end();
}

const std::string& History::Iterator::operator*() const
{
    return *mIterator;
}

History::Iterator& History::Iterator::operator--()
{
    if (mIterator != mContent.begin())
    {
        --mIterator;
    }
    return *this;
}

History::Iterator& History::Iterator::operator++()
{
    if (mIterator != mContent.end())
    {
        ++mIterator;
    }
    return *this;
}

void History::Iterator::reset()
{
    mIterator = mContent.end();
}

}  // namespace

enum class Completion
{
    forward,
    backward,
};

DEFINE_BITFLAG(Flags, char,
{
    suggestionsEnabled,
    historyEnabled,
});

struct Readline::Impl
{
    void clear();
    void clearHistory();

    bool handleKeyPress(KeyPress keyPress, InputSource source, Context& context);
    void refreshCompletion();
    void refreshSuggestion();

    void onAccept(OnAccept onAccept);

    void setupCompletion(RefreshCompletion refreshCompletion);
    void enableSuggestions();
    void disableHistory();

    const std::string& lineRef() const;
    const size_t& cursorRef() const;
    const std::string& suggestionRef() const;

    const utils::StringViews& completions() const;
    const utils::StringViews::iterator& currentCompletion() const;

private:
    void clearCompletions();
    void moveCursorLeft();
    bool moveCursorRight();
    bool write(char c);
    bool write(const std::string& string);
    void selectPrevHistoryEntry();
    void selectNextHistoryEntry();
    void jumpToBeginning();
    void jumpToEnd();
    void jumpToPrevWord();
    void jumpToNextWord();
    bool erasePrevCharacter();
    bool eraseNextCharacter();
    bool eraseWord();
    void accept(InputSource source, Context& context);
    void complete(Completion type);

    std::string                  mLine;
    size_t                       mCursor = 0;
    Flags                        mFlags = Flags::historyEnabled;
    History                      mHistory;
    std::string                  mSavedLine;
    utils::StringViews           mCompletions;
    utils::StringViews::iterator mCurrentCompletion;
    OnAccept                     mOnAccept;
    RefreshCompletion            mRefreshCompletion;
    std::string                  mSuggestion;
};

void Readline::Impl::clear()
{
    mLine.clear();
    mCursor = 0;
    clearCompletions();
    mSuggestion.clear();
    mHistory.current.reset();
}

void Readline::Impl::clearHistory()
{
    mHistory.clear();
}

bool Readline::Impl::handleKeyPress(KeyPress keyPress, InputSource source, Context& context)
{
    bool requireCompletionUpdate{false};

    switch (keyPress.type)
    {
        case KeyPress::Type::space:
        case KeyPress::Type::character:
            requireCompletionUpdate = write(keyPress.value);
            break;

        case KeyPress::Type::arrowLeft:
            moveCursorLeft();
            break;

        case KeyPress::Type::arrowRight:
            requireCompletionUpdate = moveCursorRight();
            break;

        case KeyPress::Type::arrowUp:
            selectPrevHistoryEntry();
            break;

        case KeyPress::Type::arrowDown:
            selectNextHistoryEntry();
            break;

        case KeyPress::Type::ctrlArrowLeft:
            jumpToPrevWord();
            break;

        case KeyPress::Type::ctrlArrowRight:
            jumpToNextWord();
            break;

        case KeyPress::Type::home:
            jumpToBeginning();
            break;

        case KeyPress::Type::end:
            jumpToEnd();
            break;

        case KeyPress::Type::tab:
            complete(Completion::forward);
            break;

        case KeyPress::Type::shiftTab:
            complete(Completion::backward);
            break;

        case KeyPress::Type::backspace:
            requireCompletionUpdate = erasePrevCharacter();
            break;

        case KeyPress::Type::del:
            requireCompletionUpdate = eraseNextCharacter();
            break;

        case KeyPress::Type::ctrlCharacter:
            switch (keyPress.value)
            {
                case 'c':
                    clear();
                    return true;

                case 'w':
                    requireCompletionUpdate = eraseWord();
                    break;
            }
            break;

        case KeyPress::Type::cr:
            accept(source, context);
            return true;

        default:
            requireCompletionUpdate = write(keyPress.name());
            break;
    }

    if (requireCompletionUpdate and source == InputSource::user)
    {
        refreshCompletion();
        refreshSuggestion();
    }

    return false;
}

void Readline::Impl::refreshCompletion()
{
    if (mRefreshCompletion)
    {
        mCompletions = mRefreshCompletion(mLine);
        mCurrentCompletion = mCompletions.end();
    }
}

void Readline::Impl::refreshSuggestion()
{
    if (mFlags & Flags::suggestionsEnabled)
    {
        for (std::string_view suggestion : mHistory.content | std::ranges::views::reverse)
        {
            if (suggestion.starts_with(mLine))
            {
                suggestion.remove_prefix(mLine.size());
                mSuggestion = suggestion;
                return;
            }
        }
        mSuggestion.clear();
    }
}

void Readline::Impl::onAccept(OnAccept onAccept)
{
    mOnAccept = std::move(onAccept);
}

void Readline::Impl::setupCompletion(RefreshCompletion refreshCompletion)
{
    mRefreshCompletion = std::move(refreshCompletion);
}

void Readline::Impl::enableSuggestions()
{
    mFlags |= Flags::suggestionsEnabled;
}

void Readline::Impl::disableHistory()
{
    mFlags &= ~Flags::historyEnabled;
}

const std::string& Readline::Impl::lineRef() const
{
    return mLine;
}

const size_t& Readline::Impl::cursorRef() const
{
    return mCursor;
}

const std::string& Readline::Impl::suggestionRef() const
{
    return mSuggestion;
}

const utils::StringViews& Readline::Impl::completions() const
{
    return mCompletions;
}

const utils::StringViews::iterator& Readline::Impl::currentCompletion() const
{
    return mCurrentCompletion;
}

void Readline::Impl::clearCompletions()
{
    mCompletions.clear();
    mCurrentCompletion = mCompletions.end();
}

void Readline::Impl::moveCursorLeft()
{
    if (mCursor > 0)
    {
        --mCursor;
    }
}

bool Readline::Impl::moveCursorRight()
{
    if (mCursor < mLine.size())
    {
        ++mCursor;
    }
    else if (mFlags & Flags::suggestionsEnabled and not mSuggestion.empty())
    {
        return write(mSuggestion);
    }
    return false;
}

bool Readline::Impl::write(char c)
{
    mLine.insert(mCursor++, 1, c);
    return true;
}

bool Readline::Impl::write(const std::string& string)
{
    mLine.insert(mCursor, string);
    mCursor += string.size();
    return true;
}

void Readline::Impl::selectPrevHistoryEntry()
{
    if (not (mFlags & Flags::historyEnabled))
    {
        return;
    }
    if (mHistory.current.isBeginning())
    {
        return;
    }
    if (not mHistory.current)
    {
        mSavedLine = mLine;
    }
    --mHistory.current;
    mLine = *mHistory.current;
    mCursor = mLine.size();
    clearCompletions();
}

void Readline::Impl::selectNextHistoryEntry()
{
    if (not (mFlags & Flags::historyEnabled))
    {
        return;
    }
    if (++mHistory.current)
    {
        mLine = *mHistory.current;
        mCursor = mLine.size();
    }
    else
    {
        mLine = mSavedLine;
        mCursor = mLine.size();
    }
    clearCompletions();
    mSuggestion.clear();
}

void Readline::Impl::jumpToBeginning()
{
    mCursor = 0;
}

void Readline::Impl::jumpToEnd()
{
    mCursor = mLine.size();
}

void Readline::Impl::jumpToPrevWord()
{
    if (mCursor > 0)
    {
        if (mLine[mCursor - 1] == ' ')
        {
            --mCursor;
        }
        if (mCursor > 0)
        {
            auto it = mLine.rfind(' ', --mCursor);
            mCursor = it != std::string::npos
                ? it
                : 0;
        }
    }
}

void Readline::Impl::jumpToNextWord()
{
    if (mCursor < mLine.size())
    {
        auto it = mLine.find(' ', ++mCursor);
        mCursor = it != std::string::npos
            ? it
            : mLine.size();
    }
}

bool Readline::Impl::erasePrevCharacter()
{
    if (mCursor > 0)
    {
        mLine.erase(--mCursor, 1);
        return true;
    }
    return false;
}

bool Readline::Impl::eraseNextCharacter()
{
    if (mCursor < mLine.size())
    {
        mLine.erase(mCursor, 1);
        return true;
    }
    return false;
}

bool Readline::Impl::eraseWord()
{
    if (mCursor > 0)
    {
        auto it = mLine.rfind(' ', mCursor - 1);
        if (it == std::string::npos)
        {
            it = 0;
        }
        else
        {
            while (it)
            {
                if (mLine[it - 1] != ' ')
                {
                    break;
                }
                it--;
            }
        }
        mLine.erase(it, mCursor - it);
        mCursor = it;
        return true;
    }
    return false;
}

void Readline::Impl::accept(InputSource source, Context& context)
{
    if (source == InputSource::user and (mFlags & Flags::historyEnabled))
    {
        mHistory.pushBack(mLine);
    }
    if (mOnAccept)
    {
        mOnAccept(source, context);
    }
}

void Readline::Impl::complete(Completion type)
{
    if (not mCompletions.empty())
    {
        utils::StringViews::iterator pivot;
        utils::StringViews::iterator nextPivot;
        utils::StringViews::iterator next;

        switch (type)
        {
            case Completion::forward:
                pivot = mCompletions.end();
                nextPivot = mCompletions.begin();
                next = mCurrentCompletion + 1;
                break;
            case Completion::backward:
                pivot = mCompletions.begin();
                nextPivot = mCompletions.end();
                next = mCurrentCompletion - 1;
                break;
        }

        if (mCurrentCompletion == pivot)
        {
            mCurrentCompletion = nextPivot;
        }
        else
        {
            mCurrentCompletion = next;
        }

        if (mCurrentCompletion != mCompletions.end())
        {
            mLine = *mCurrentCompletion;
            mCursor = mLine.size();
        }
    }
}

Readline::Readline()
    : mPimpl(new Impl)
{
}

Readline::~Readline()
{
    delete mPimpl;
}

void Readline::clear()
{
    mPimpl->clear();
}

void Readline::clearHistory()
{
    mPimpl->clearHistory();
}

bool Readline::handleKeyPress(KeyPress keyPress, InputSource source, Context& context)
{
    return mPimpl->handleKeyPress(keyPress, source, context);
}

void Readline::refreshCompletion()
{
    mPimpl->refreshCompletion();
}

Readline& Readline::onAccept(OnAccept onAccept)
{
    mPimpl->onAccept(std::move(onAccept));
    return *this;
}

Readline& Readline::setupCompletion(RefreshCompletion refreshCompletion)
{
    mPimpl->setupCompletion(std::move(refreshCompletion));
    return *this;
}

Readline& Readline::enableSuggestions()
{
    mPimpl->enableSuggestions();
    return *this;
}

Readline& Readline::disableHistory()
{
    mPimpl->disableHistory();
    return *this;
}

const std::string& Readline::lineRef() const
{
    return mPimpl->lineRef();
}

const size_t& Readline::cursorRef() const
{
    return mPimpl->cursorRef();
}

const std::string& Readline::suggestionRef() const
{
    return mPimpl->suggestionRef();
}

const utils::StringViews& Readline::completions() const
{
    return mPimpl->completions();
}

const utils::StringViews::iterator& Readline::currentCompletion() const
{
    return mPimpl->currentCompletion();
}

}  // namespace core
