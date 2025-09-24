#include "readline.hpp"

#include <flat_set>
#include <ranges>

#include "core/input.hpp"
#include "core/picker.hpp"
#include "utils/bitflag.hpp"
#include "utils/hash_map.hpp"
#include "utils/immobile.hpp"
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
        Iterator(utils::Strings& content)
            : mContent(content)
            , mIterator(mContent.end())
        {
        }

        bool isBeginning() const
        {
            return mIterator == mContent.begin();
        }

        operator bool() const
        {
            return mIterator != mContent.end();
        }

        const std::string& operator*() const
        {
            return *mIterator;
        }

        Iterator& operator--()
        {
            if (mIterator != mContent.begin())
            {
                --mIterator;
            }
            return *this;
        }

        Iterator& operator++()
        {
            if (mIterator != mContent.end())
            {
                ++mIterator;
            }
            return *this;
        }

        void reset()
        {
            mIterator = mContent.end();
        }

    private:
        utils::Strings&  mContent;
        utils::StringsIt mIterator;
    };

    History()
        : current(content)
    {
    }

    void pushBack(const std::string& entry)
    {
        auto result = mContentSet.emplace(entry);

        if (result.second)
        {
            content.push_back(entry);
            current.reset();
        }
    }

    void clear()
    {
        content.clear();
        current.reset();
    }

    utils::Strings content;
    Iterator       current;

private:
    StringSet      mContentSet;
};

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
    pickerAlwaysOn,
});

struct Readline::Impl final : utils::Immobile
{
    void clear();
    void clearHistory();

    bool handleKeyPress(KeyPress keyPress, InputSource source, Context& context);
    void refreshCompletion();
    void refreshSuggestion();
    void refreshFuzzy();

    void onAccept(OnAccept onAccept);

    void setupCompletion(RefreshCompletion refreshCompletion);
    void enableSuggestions();
    void disableHistory();
    void setPageSize(size_t pageSize);
    void connectPicker(Picker& picker, char ctrlCharacter, AcceptBehaviour acceptBehaviour);
    void connectPicker(Picker& picker, Context& context);

    const std::string& line() const;
    const size_t& cursor() const;
    const std::string& suggestion() const;

    const utils::StringViews& completions() const;
    const utils::StringViewsIt& currentCompletion() const;
    const Picker* picker() const;
    const utils::Strings& history() const;

private:
    void saveLine();
    void saveAndClearLine();
    void restoreLine();
    void copyToClipboard(size_t start, size_t end);
    bool pasteFromClipboard();
    void clearCompletions();
    void moveCursorLeft();
    bool moveCursorRight();
    bool write(char c);
    bool write(const std::string& string);
    void selectPrevHistoryEntry();
    bool selectNextHistoryEntry();
    void jumpToBeginning();
    void jumpToEnd();
    void jumpToPrevWord();
    void jumpToNextWord();
    bool erasePrevCharacter();
    bool eraseNextCharacter();
    bool cutPrevWord();
    bool cutNextWord();
    bool accept(InputSource source, Context& context);
    void complete(Completion type);
    bool activatePicker(char c, Context& context);
    void refresh();

    struct PickerData final
    {
        Picker*         picker;
        AcceptBehaviour acceptBehaviour;
    };

    using Pickers = utils::HashMap<char, PickerData>;

    std::string          mLine;
    size_t               mCursor = 0;
    Flags                mFlags = Flags::historyEnabled;
    History              mHistory;
    std::string          mSavedLine;
    std::string          mClipboard;
    utils::StringViews   mCompletions;
    utils::StringViewsIt mCurrentCompletion;
    OnAccept             mOnAccept;
    RefreshCompletion    mRefreshCompletion;
    std::string          mSuggestion;
    Pickers              mPickers;
    Picker*              mPicker = nullptr;
    AcceptBehaviour      mPickerBehaviour = AcceptBehaviour::replace;
};

void Readline::Impl::clear()
{
    mLine.clear();
    mCursor = 0;
    clearCompletions();
    mSuggestion.clear();
    mHistory.current.reset();
    if (mPicker)
    {
        mPicker->clear();
        mPicker = nullptr;
    }
}

void Readline::Impl::clearHistory()
{
    mHistory.clear();
}

bool Readline::Impl::handleKeyPress(KeyPress keyPress, InputSource source, Context& context)
{
    bool requireRefresh{false};

    switch (keyPress.type)
    {
        case KeyPress::Type::space:
        case KeyPress::Type::character:
            requireRefresh = write(keyPress.value);
            break;

        case KeyPress::Type::arrowLeft:
            moveCursorLeft();
            break;

        case KeyPress::Type::arrowRight:
            requireRefresh = moveCursorRight();
            break;

        case KeyPress::Type::arrowUp:
            selectPrevHistoryEntry();
            break;

        case KeyPress::Type::arrowDown:
            requireRefresh = selectNextHistoryEntry();
            break;

        case KeyPress::Type::pageUp:
            if (mPicker)
            {
                mPicker->movePage(-1);
            }
            break;

        case KeyPress::Type::pageDown:
            if (mPicker)
            {
                mPicker->movePage(1);
            }
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
            requireRefresh = erasePrevCharacter();
            break;

        case KeyPress::Type::del:
            requireRefresh = eraseNextCharacter();
            break;

        case KeyPress::Type::ctrlCharacter:
            if (activatePicker(keyPress.value, context))
            {
                break;
            }
            switch (keyPress.value)
            {
                case 'a':
                    mCursor = 0;
                    break;

                case 'e':
                    mCursor = mLine.size();
                    break;

                case 'c':
                    goto exitReadline;

                case 'w':
                    requireRefresh = cutPrevWord();
                    break;

                case 'y':
                    requireRefresh = pasteFromClipboard();
                    break;
            }
            break;

        case KeyPress::Type::altCharacter:
            switch (keyPress.value)
            {
                case 'd':
                    requireRefresh = cutNextWord();
                    break;
            }
            break;

        case KeyPress::Type::cr:
            if (accept(source, context))
            {
                return true;
            }
            requireRefresh = true;
            break;

        exitReadline:
        case KeyPress::Type::escape:
            if (mPicker and not mFlags[Flags::pickerAlwaysOn])
            {
                mPicker->clear();
                mPicker = nullptr;
                restoreLine();
                return false;
            }
            return true;

        default:
            requireRefresh = write(keyPress.name());
            break;
    }

    if (requireRefresh and source == InputSource::user)
    {
        refresh();
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
    if (mFlags[Flags::suggestionsEnabled])
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

void Readline::Impl::connectPicker(Picker& picker, char ctrlCharacter, AcceptBehaviour acceptBehaviour)
{
    mPickers.insert(ctrlCharacter, PickerData{.picker = &picker, .acceptBehaviour = acceptBehaviour});
}

void Readline::Impl::connectPicker(Picker& picker, Context& context)
{
    if (mPicker)
    {
        mPicker->clear();
    }
    mPicker = &picker;
    mFlags |= Flags::pickerAlwaysOn;
    mPicker->load(context);
}

const std::string& Readline::Impl::line() const
{
    return mLine;
}

const size_t& Readline::Impl::cursor() const
{
    return mCursor;
}

const std::string& Readline::Impl::suggestion() const
{
    return mSuggestion;
}

const utils::StringViews& Readline::Impl::completions() const
{
    return mCompletions;
}

const utils::StringViewsIt& Readline::Impl::currentCompletion() const
{
    return mCurrentCompletion;
}

const Picker* Readline::Impl::picker() const
{
    return mPicker;
}

const utils::Strings& Readline::Impl::history() const
{
    return mHistory.content;
}

void Readline::Impl::saveLine()
{
    mSavedLine = mLine;
}

void Readline::Impl::saveAndClearLine()
{
    mSavedLine = std::move(mLine);
    mCursor = 0;
}

void Readline::Impl::restoreLine()
{
    mLine = std::move(mSavedLine);
    mCursor = mLine.size();
}

void Readline::Impl::copyToClipboard(size_t start, size_t end)
{
    mClipboard = std::string(mLine.begin() + start, mLine.begin() + end);
}

bool Readline::Impl::pasteFromClipboard()
{
    if (mClipboard.empty())
    {
        return false;
    }

    write(mClipboard);
    return true;
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
    else if (mFlags[Flags::suggestionsEnabled] and not mSuggestion.empty())
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
    if (mPicker)
    {
        mPicker->move(-1);
        return;
    }

    if (not mFlags[Flags::historyEnabled] or mHistory.current.isBeginning())
    {
        return;
    }

    if (not mHistory.current)
    {
        saveLine();
    }

    --mHistory.current;
    mLine = *mHistory.current;
    mCursor = mLine.size();

    clearCompletions();
    mSuggestion.clear();
}

bool Readline::Impl::selectNextHistoryEntry()
{
    if (mPicker)
    {
        mPicker->move(1);
        return false;
    }

    if (not mFlags[Flags::historyEnabled] or not mHistory.current)
    {
        return false;
    }

    if (++mHistory.current)
    {
        mLine = *mHistory.current;
        mCursor = mLine.size();

        clearCompletions();
        mSuggestion.clear();
        return false;
    }

    restoreLine();
    return true;
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
            mCursor = it != mLine.npos
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
        mCursor = it != mLine.npos
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

bool Readline::Impl::cutPrevWord()
{
    if (mCursor > 0)
    {
        auto it = mLine.rfind(' ', mCursor - 1);
        if (it == mLine.npos)
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
        copyToClipboard(it, mCursor);
        mLine.erase(it, mCursor - it);
        mCursor = it;
        return true;
    }
    return false;
}

bool Readline::Impl::cutNextWord()
{
    auto size = mLine.size();

    if (size > 0 and mCursor < mLine.size() - 1)
    {
        auto it = mLine.find(' ', mCursor + 1);

        if (it == mLine.npos)
        {
            copyToClipboard(mCursor, size);
            mLine.erase(mCursor, size - mCursor);
        }
        else
        {
            copyToClipboard(mCursor, it);
            mLine.erase(mCursor, it - mCursor);
        }

        return true;
    }

    return false;
}

bool Readline::Impl::accept(InputSource source, Context& context)
{
    if (mPicker)
    {
        auto line = mPicker->atCursor();

        if (line)
        {
            switch (mPickerBehaviour)
            {
                case AcceptBehaviour::append:
                    restoreLine();
                    write(*line);
                    break;

                case AcceptBehaviour::replace:
                    mLine = *line;
                    mCursor = mLine.size();
                    break;
            }
        }

        mPicker->clear();
        mPicker = nullptr;

        if (not mFlags[Flags::pickerAlwaysOn])
        {
            return false;
        }
    }

    if (source == InputSource::user and mFlags[Flags::historyEnabled] and not mLine.empty())
    {
        mHistory.pushBack(mLine);
    }

    if (mOnAccept)
    {
        mOnAccept(source, context);
    }

    return true;
}

void Readline::Impl::complete(Completion type)
{
    if (not mCompletions.empty() and not mPicker)
    {
        utils::StringViewsIt pivot;
        utils::StringViewsIt nextPivot;
        utils::StringViewsIt next;

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

        if (mCurrentCompletion == mCompletions.end())
        {
            saveLine();
        }

        mCurrentCompletion = mCurrentCompletion == pivot
            ? nextPivot
            : next;

        if (mCurrentCompletion != mCompletions.end())
        {
            mLine = *mCurrentCompletion;
            mCursor = mLine.size();
            mSuggestion.clear();
        }
        else
        {
            restoreLine();
        }
    }
}

bool Readline::Impl::activatePicker(char c, Context& context)
{
    if (mFlags[Flags::pickerAlwaysOn])
    {
        return false;
    }

    auto pickerIt = mPickers.find(c);

    if (not pickerIt)
    {
        return false;
    }

    if (mPicker)
    {
        mPicker->clear();
    }

    saveAndClearLine();
    auto& data = pickerIt->second;
    mPicker = data.picker;
    mPickerBehaviour = data.acceptBehaviour;
    mPicker->load(context);

    return true;
}

void Readline::Impl::refresh()
{
    if (mPicker)
    {
        mPicker->filter(mLine);
        mSuggestion.clear();
    }
    else
    {
        refreshCompletion();
        refreshSuggestion();
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

Readline& Readline::connectPicker(Picker& picker, char ctrlCharacter, AcceptBehaviour acceptBehaviour)
{
    mPimpl->connectPicker(picker, ctrlCharacter, acceptBehaviour);
    return *this;
}

Readline& Readline::connectPicker(Picker& picker, Context& context)
{
    mPimpl->connectPicker(picker, context);
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

const std::string& Readline::line() const
{
    return mPimpl->line();
}

const size_t& Readline::cursor() const
{
    return mPimpl->cursor();
}

const std::string& Readline::suggestion() const
{
    return mPimpl->suggestion();
}

const utils::StringViews& Readline::completions() const
{
    return mPimpl->completions();
}

const utils::StringViewsIt& Readline::currentCompletion() const
{
    return mPimpl->currentCompletion();
}

const Picker* Readline::picker() const
{
    return mPimpl->picker();
}

const utils::Strings& Readline::history() const
{
    return mPimpl->history();
}

}  // namespace core
