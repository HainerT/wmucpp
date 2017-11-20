/*
 * WMuCpp - Bare Metal C++ 
 * Copyright (C) 2016, 2017 Wilhelm Meier <wilhelm.wm.meier@googlemail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "std/array"
#include "container/stringbuffer.h"
#include "container/pgmstring.h"

namespace Hott {
    static constexpr uint8_t MenuLength = 7;
    static constexpr uint8_t MenuStringLength = 20;
    using MenuString = StringBuffer<MenuStringLength>;
    
    using BufferString = StringBuffer<MenuStringLength + 1>;
    using Display = std::array<BufferString, MenuLength + 1>;
    
    class MenuItem {
    public:
        bool isSelected() const {return mSelected;}
        
        virtual void putTextInto(BufferString& buffer) const = 0;
        virtual MenuItem* processKey(Hott::key_t) {return this;}
        virtual bool hasChildren() const {return false;}
    protected:
        bool mSelected = false;
    private:
//        virtual ~MenuItem() {};
    };

    template<uint8_t Length, typename C>
    class span {
    public:
        explicit span(C* data) : mData{data} {}
        C& operator[](uint8_t index) {
            assert(index < Length);
            return mData[index];   
        }
    private:
        C* mData;
    };
    
    template<uint8_t Offset, uint8_t Length, typename C>
    span<Length, typename C::type> make_span(C& c) {
        static_assert((Offset + Length) <= C::size);
        return span<Length, typename C::type>(&c[Offset]);
    }
    
    template<typename Key, uint8_t Max, typename Provider>
    class TextWithValue : public MenuItem {
    public:
        TextWithValue(const PgmStringView& text, Provider& provider, Key key) : mTitle(text), mProvider(provider), mKey(key) {}
        
        
        virtual void valueToText(uint8_t value, span<3, char> buffer) const {
            Util::itoa_r<10>(value, buffer.mData);
        }
//        virtual void valueToText(uint8_t value, char* buffer) const {
//            Util::itoa_r<10>(value, buffer);
//        }
        
        virtual void putTextInto(BufferString& buffer) const override {
            buffer[0] = ' ';
            buffer.insertAtFill(1, mTitle);
            
            auto& value = mProvider[mKey];
            if (value) {
//                valueToText(*value, span(buffer));
//                valueToText(*value, span<3, char>(&buffer[18]));
                valueToText(*value, make_span<18, 3>(buffer));
            }
            else {
                buffer[18] = '-';
                buffer[19] = '-';
                buffer[20] = '-';
            }
            
            if (mSelected) {
                buffer[18] |= 0x80;
                buffer[19] |= 0x80;
                buffer[20] |= 0x80;
            }
        }
        virtual MenuItem* processKey(Hott::key_t key) override {
            auto& v = mProvider[mKey];
            switch (key) {
            case Hott::key_t::up:
                if (mSelected) {
                    if (v) {
                        if (*v > 0) {
                            --v;
                        }
                        else {
                            v.setNaN();
                        }
                    }
                    else {
                        *v = 0;
                    }
                }
                break;
            case Hott::key_t::down:
                if (mSelected) {
                    if (v) {
                        if (*v < Max) {
                            ++v;
                        }
                        else {
                            v.setNaN();
                        }
                    }
                    else {
                        *v = 0;
                    }
                }
                break;
            case Hott::key_t::left:
                break;
            case Hott::key_t::right:
                break;
            case Hott::key_t::set:
                if (mSelected) {
                    mProvider.change();
                }
                mSelected = !mSelected;
                break;
            }
            return this;
        }
    private:
        const PgmStringView mTitle;
        Provider& mProvider;
        const Key mKey;
    };
    
    class Menu : public MenuItem {
    public:
        template<typename... T>
        Menu(Menu* parent, const PgmStringView& title, T*... items) : mParent(parent), mTitle(title), mItems{items...} {}
        
        virtual bool hasChildren() const override {return true;}
        
        virtual void titleInto(BufferString& buffer) const {
            buffer.insertAtFill(0, mTitle);
        }
        
        virtual void putTextInto(BufferString& buffer) const override {
            buffer[0] = ' ';
            buffer.insertAtFill(1, mTitle);
        }
        void textTo(Display& display) const {
            static uint_ranged_circular<uint8_t, 0, Display::size - 1> line;
            if (line == 0) {
                titleInto(display[0]);
                ++line;
            }
            else {
                if (lineToDisplay(display[line], line)) {
                    if (mSelectedLine && (mSelectedLine == (line - 1))) {
                        display[mSelectedLine + 1][0] = '>';
                    }
                    ++line;
                }
            }
        }
        bool lineToDisplay(BufferString& buffer, uint8_t row) const {
            if (mItems[row - 1]) {
                mItems[row - 1]->putTextInto(buffer);
            }
            return true;
        }
        Menu* processKey(Hott::key_t key) override {
            if (mSelectedLine) {
                if (mItems[mSelectedLine]->isSelected()) {
                    mItems[mSelectedLine]->processKey(key);
                    return this;
                }
            }
            switch (key) {
            case Hott::key_t::down:
                if (mSelectedLine) {
                    if (mItems[mSelectedLine + 1]) {
                        ++mSelectedLine;
                    }
                }
                else {
                    mSelectedLine = 0;
                }
                break;
            case Hott::key_t::up:
                if (mSelectedLine) {
                    --mSelectedLine;
                }
                else {
                    mSelectedLine = 0;
                }
                break;
            case Hott::key_t::left:
                if (mParent) {
                    return mParent;
                }
                break;
            case Hott::key_t::right:
                break;
            case Hott::key_t::set:
                if (mSelectedLine) {
                    if (mItems[mSelectedLine]->hasChildren()) {
                        return static_cast<Menu*>(mItems[mSelectedLine]);
                    }        
                    else {
                        mItems[mSelectedLine]->processKey(key);
                    }
                }
                break;
            }
            return this;
        }
    private:
        Menu* mParent = nullptr;
        const PgmStringView mTitle;
        const std::array<MenuItem*, MenuLength> mItems;
        uint_ranged_NaN<uint8_t, 0, 7> mSelectedLine;
    };
    
    class NumberedMenu : public Menu {
    public:
        template<typename...T>
        NumberedMenu(Menu* parent, uint8_t number, const PgmStringView& title, T*... items) : Menu(parent, title, items...), 
            mNumber(number) {}
        virtual void titleInto(BufferString& buffer) const override{
            buffer.insertAtFill(0, mTitle);
            buffer[19] =  '0' + mNumber;
        }
        
        virtual void putTextInto(BufferString& buffer) const override {
            buffer[0] = ' ';
            buffer.insertAtFill(1, mTitle);
            buffer[19] =  '0' + mNumber;
        }
    private:
        uint8_t mNumber;
    };
}
