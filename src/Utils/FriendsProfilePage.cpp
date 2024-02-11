#include "../includes.h"
#include "../BrownAlertDelegate.hpp"
#include <Geode/modify/FriendsProfilePage.hpp>
// Utils

// Function to convert a const char* to lowercase
const char* toLowerCase(const char* str) {
    char* result = new char[strlen(str) + 1];
    size_t i = 0;

    while (str[i]) {
        result[i] = std::tolower(str[i]);
        i++;
    }

    result[i] = '\0';
    return result;
}

class SearchUserLayer : public BrownAlertDelegate {
    protected:
        virtual void setup() {
            auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();
            input_username->setPositionY(10);
            this->m_buttonMenu->addChild(input_username);
            auto validate_spr = ButtonSprite::create("Search", 60, true, "bigFont.fnt", "GJ_button_01.png", 30, .5F);
            auto validate_btn = CCMenuItemSpriteExtra::create(
                validate_spr,
                this,
                menu_selector(SearchUserLayer::onValidate)
            );

            validate_btn->setPosition({
                0,
                -35
            });
            this->m_buttonMenu->addChild(validate_btn, 1);
            this->m_mainLayer->addChild(this->m_buttonMenu);
            setTouchEnabled(true);
        }
        cocos2d::CCSize m_sScrLayerSize;
        void onClose(cocos2d::CCObject* pSender) {
            BrownAlertDelegate::onClose(pSender);
        }
        void onValidate(cocos2d::CCObject*);
        float m_fWidth = s_defWidth;
        float m_fHeight = s_defHeight;
    public:
        InputNode* input_username = InputNode::create(200.0F, "Username", "bigFont.fnt", "", 20);
        static constexpr const float s_defWidth = 260.0f;
        static constexpr const float s_defHeight = 120.0f;
        static SearchUserLayer* create() {
            auto pRet = new SearchUserLayer();
            if (pRet && pRet->init(SearchUserLayer::s_defWidth, SearchUserLayer::s_defHeight, "GJ_square01.png")) {
                pRet->autorelease();
                return pRet;
            }
            CC_SAFE_DELETE(pRet);
            return nullptr;
        }
};

class $modify(FriendPage, FriendsProfilePage) {
    bool init(UserListType type) {
        if (!FriendsProfilePage::init(type)) return false;
        if (!Mod::get()->getSettingValue<bool>("friendSearch")) return true;
        auto menu = this->m_buttonMenu;

        auto downSpr = CCSprite::createWithSpriteFrameName("edit_downBtn2_001.png");
        auto downBtn = CCMenuItemSpriteExtra::create(
            downSpr,
            this,
            menu_selector(FriendPage::onDown)
        );
        downBtn->setPosition(404, -160);
        menu->addChild(downBtn);

        auto upSpr = CCSprite::createWithSpriteFrameName("edit_upBtn2_001.png");
        auto upBtn = CCMenuItemSpriteExtra::create(
            upSpr,
            this,
            menu_selector(FriendPage::onUp)
        );
        upBtn->setPosition(404, -130);
        menu->addChild(upBtn);

        auto searchSpr = CCSprite::createWithSpriteFrameName("gj_findBtn_001.png");
        auto searchBtn = CCMenuItemSpriteExtra::create(
            searchSpr,
            this,
            menu_selector(FriendPage::onSearch)
        );
        searchBtn->setPosition(404, -88);
        menu->addChild(searchBtn);
        return true;
    }
    
    
    void onDown(CCObject*) {
        // JOUCA WHAT IS THIS LOL
        auto scene = CCDirector::sharedDirector()->getRunningScene();
        auto sceneChildren = scene->getChildren();
        auto customList = getCustomList(sceneChildren);

        customList->scrollLayer(200);
    }
    void onUp(CCObject*) {
        // NANI
        auto scene = CCDirector::sharedDirector()->getRunningScene();
        auto sceneChildren = scene->getChildren();
        auto customList = getCustomList(sceneChildren);
        customList->scrollLayer(-200);
    }
    void onSearch(CCObject*) {
        SearchUserLayer::create()->show();
    }
    static TableView* getCustomList(CCArray* sceneChildren) {
        CCLayer* test1 = typeinfo_cast<CCLayer*>(misc::findNode("FriendsProfilePage"));
        if (test1 == nullptr) {
            // safeguard from crashing
            FLAlertLayer::create(nullptr,
                "Error",
                "The mod could not find the <cy>FriendsProfilePage</c> layer. Please either <cg>try again later</c>, removing mods that may be interfering with the scene, or report this to the developers.",
                "OK",
                nullptr,
                350.0F
            )->show();
            return nullptr;
        }
        test1 = typeinfo_cast<CCLayer*>(test1->getChildren()->objectAtIndex(0));
        CCLayer* test2 = nullptr;
        for (int i = 0; i < test1->getChildrenCount(); i++) {
            if (misc::getNodeName(test1->getChildren()->objectAtIndex(i)) == "GJCommentListLayer") {
                test2 = static_cast<CCLayer*>(test1->getChildren()->objectAtIndex(i));
                break;
            }
        }
        if (test2 == nullptr) {
            // safeguard from crashing
            FLAlertLayer::create(nullptr,
                "Error",
                "The mod could not find the <cy>GJCommentListLayer</c> layer. Please either <cg>try again later</c>, removing mods that may be interfering with the scene, or report this to the developers.",
                "OK",
                nullptr,
                350.0F
            )->show();
            return nullptr;
        }

        auto test3 = static_cast<CCLayer*>(test2->getChildren()->objectAtIndex(0));
        
        if (test3->getChildrenCount() <= 0) {
            // another safeguard
            FLAlertLayer::create(nullptr,
                "Error",
                "You have <cy>no friends</c>!",
                "OK",
                nullptr,
                200.0F
            )->show();
            return nullptr;
        }
        return static_cast<TableView*>(test3->getChildren()->objectAtIndex(0));
    }
    static void searchUser(const char* username) {
        auto scene = CCDirector::sharedDirector()->getRunningScene();
        auto sceneChildren = scene->getChildren();
        auto customList = getCustomList(sceneChildren);
        CCContentLayer* contentLayer = static_cast<CCContentLayer*>(
            customList->getChildren()->objectAtIndex(0)
        );
        int counter_page = 0;
        bool found = false;
        for (int i = 0; i < contentLayer->getChildrenCount(); i++) {
            CCMenu* cell;
            CCLabelBMFont* label;
            cell = typeinfo_cast<CCMenu*>(
                reinterpret_cast<CCLayer*>(
                    reinterpret_cast<CCLayer*>(
                        reinterpret_cast<CCLayer*>(
                            contentLayer->getChildren()->objectAtIndex(i)
                        )
                    )->getChildren()->objectAtIndex(1)
                )->getChildren()->objectAtIndex(0)
            );

            if (cell == nullptr) {
                label = reinterpret_cast<CCLabelBMFont*>(
                    reinterpret_cast<CCLayer*>(
                        reinterpret_cast<CCLayer*>(
                            reinterpret_cast<CCLayer*>(
                                reinterpret_cast<CCLayer*>(
                                    reinterpret_cast<CCLayer*>(
                                        contentLayer->getChildren()->objectAtIndex(i)
                                    )
                                )->getChildren()->objectAtIndex(1)
                            )->getChildren()->objectAtIndex(1)
                        )->getChildren()->objectAtIndex(0)
                    )->getChildren()->objectAtIndex(0)
                );
            } else {
                label = reinterpret_cast<CCLabelBMFont*>(
                    reinterpret_cast<CCLayer*>(
                        reinterpret_cast<CCLayer*>(
                            cell->getChildren()->objectAtIndex(0)
                        )->getChildren()->objectAtIndex(0)
                    )
                );
            }
            const char* str1 = label->getString();
            if (strstr(toLowerCase(str1), toLowerCase(username)) != nullptr) {
                customList->scrollLayer(-9999999);
                customList->scrollLayer(counter_page);

                found = true;

                break;
            }
            counter_page += 45;
        }
        if (!found) {
            std::string str = username;
            FLAlertLayer::create(nullptr,
                "Error",
                "User not found : " + str,
                "OK",
                nullptr,
                200.0F
            )->show();
        }
    }
};

void SearchUserLayer::onValidate(CCObject* pSender) {
    FriendPage::searchUser(input_username->getString().c_str());
    BrownAlertDelegate::onClose(pSender);
}

// Utils
