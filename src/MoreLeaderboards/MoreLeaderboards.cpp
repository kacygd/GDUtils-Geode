#include "MoreLeaderboards.h"
#include "Newtab.h"
#include <Geode/modify/LeaderboardsLayer.hpp>
#include <Geode/ui/GeodeUI.hpp>

StatsListType MoreLeaderboards::g_tab = StatsListType::Stars;
static int page = 0;
static int start_count = 0;
static int end_count = 0;
static int total_count = 0;
std::string MoreLeaderboards::data_response_moreLB = "";
static std::unordered_map<std::string, web::WebTask> RUNNING_REQUESTS {};

std::vector<std::string> MoreLeaderboards::getWords(std::string s, std::string delim) {
    std::vector<std::string> res;
    std::string token = "";
    for (int i = 0; i < s.size(); i++) {
        bool flag = true;
        for (int j = 0; j < delim.size(); j++) {
            if (s[i + j] != delim[j]) flag = false;
        }
        if (flag) {
            if (token.size() > 0) {
                res.push_back(token);
                token = "";
                i += delim.size() - 1;
            }
        } else {
            token += s[i];
        }
    }
    res.push_back(token);
    return res;
}

class SearchUserLBLayer : public BrownAlertDelegate {
    protected:
        MoreLeaderboards* m_layer;

        virtual void setup() {
            auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();
            InputNode* input_username = InputNode::create(200.0F, "Username", "bigFont.fnt", "", 20);
            input_username->setPositionY(10);
            this->m_buttonMenu->addChild(input_username);
            auto validate_spr = ButtonSprite::create("Search", 60, true, "bigFont.fnt", "GJ_button_01.png", 30, .5F);
            auto validate_btn = CCMenuItemSpriteExtra::create(
                validate_spr,
                this,
                menu_selector(SearchUserLBLayer::onValidate)
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
        static constexpr const float s_defWidth = 260.0f;
        static constexpr const float s_defHeight = 120.0f;
        static SearchUserLBLayer* create(MoreLeaderboards* layer) {
            auto pRet = new SearchUserLBLayer();
            if (pRet && pRet->init(SearchUserLBLayer::s_defWidth, SearchUserLBLayer::s_defHeight, "GJ_square01.png")) {
                pRet->autorelease();
                pRet->m_layer = layer;
                return pRet;
            }
            CC_SAFE_DELETE(pRet);
            return nullptr;
        }
};

void SearchUserLBLayer::onValidate(CCObject* pSender) {
    if (this->m_layer->loading) return;

    this->m_layer->changeTabPage();

    this->m_layer->onTab(nullptr);

    BrownAlertDelegate::onClose(pSender);
}

CCDictionary* MoreLeaderboards::responseToDict(const std::string& response){
    auto dict = CCDictionary::create();
    std::stringstream responseStream(response);
    std::string currentKey;
    std::string keyID;
    std::string playerID;
    std::string username;
    std::string accountID;

    GameLevelManager* glm = GameLevelManager::sharedState();
    unsigned int i = 0;
    while(getline(responseStream, currentKey, ':')){
        if(i % 2 == 0) {
            keyID = currentKey;
        } else {
            if (keyID == "2") { // Player ID
                playerID = currentKey;
            } else if (keyID == "16") {
                accountID = currentKey;
            } else if (keyID == "1") {
                username = currentKey;
            }
            dict->setObject(CCString::create(currentKey.c_str()),keyID);
        }
        i++;
    }
    if (playerID.length() > 0 && username.length() > 0 && accountID.length() > 0) {
        glm->storeUserName(std::stoi(playerID), std::stoi(accountID), username);
    }
    return dict;
}

void MoreLeaderboards::onMoreLeaderboards(CCObject* pSender) {
    auto scene = CCScene::create();
    auto layer = MoreLeaderboards::create("more");
    scene->addChild(layer);
    auto transitionFade = CCTransitionFade::create(0.5, scene);
    CCDirector::sharedDirector()->pushScene(transitionFade);
}

void MoreLeaderboards::onModsList(CCObject* pSender) {
    auto scene = CCScene::create();
    auto layer = MoreLeaderboards::create("mods");
    scene->addChild(layer);
    auto transitionFade = CCTransitionFade::create(0.5, scene);
    CCDirector::sharedDirector()->pushScene(transitionFade);
}

void MoreLeaderboards::onRegion(CCObject* pSender) {
    SelectRegion::scene([this](int id) {
        country_id = id;
        
        if (loading) return;

        loading = true;

        loading_circle = LoadingCircle::create();
        loading_circle->setZOrder(25);
        loading_circle->setParentLayer(this);
        loading_circle->show();

        if (displayedData) {
            displayedData->release();
            displayedData = cocos2d::CCArray::create();
            displayedData->retain();
        }

        page = 0;
        resetInfos();
        
        if (loading) {
            startLoadingMore();
            loadPageMore();
        }
    });
    /*geode::createQuickPopup(
        "Coming soon!",
        R"text(
<cy>Filter by country</c> leaderboards is coming soon on GDUtils!
Stay tuned for updates via our <cj>Discord server</c> on the page of the mod in the <cy>Geode Index</c>!
        )text",
        "OK", "Mod page",
        [](auto, bool btn2) {
            if (btn2) {
                openIndexPopup(Mod::get());
            }
        }
    );*/
}

void MoreLeaderboards::onInfo(CCObject* pSender) {
    FLAlertLayer::create(
        nullptr,
        "More Leaderboards",
        R"text(
<cy>More Leaderboards</c> is a feature that allows you to view more leaderboards than the ones in the game.
You can view leaderboards for <cj>Diamonds</c>, User Coins, <cr>Demons</c>, <cl>Moons</c>, and Creators Points of people that are registered on <cy>Updated Leaderboards</c> project.

Thanks to <cj>ChiefWoof</c> & <co>the Helper team</c> for providing the data for this feature!
        )text",
        "OK",
        nullptr,
        400.0f
    )->show();
}

void MoreLeaderboards::onLBInfo(CCObject* pSender) {
    geode::createQuickPopup(
        "Submit your user stats",
        R"text(
You can submit your <cy>user stats</c> to the <cj>Updated Leaderboards</c> team by joining their <cb>Discord server</c> and following the instructions right there.
        )text",
        "OK", "Discord Server",
        [](auto, bool btn2) {
            if (btn2) {
                web::openLinkInBrowser("https://discord.gg/mZnDm886sR");
            }
        }
    );
}

MoreLeaderboards* MoreLeaderboards::create(std::string type) {
    auto pRet = new MoreLeaderboards();
    SelectRegionCell::country_id = 0;
    if (pRet && pRet->init(type)) {
        pRet->autorelease();
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return nullptr;
}

bool MoreLeaderboards::init(std::string type) {
    auto director = cocos2d::CCDirector::sharedDirector();
    auto size = director->getWinSize();

    displayedData = cocos2d::CCArray::create();
    displayedData->retain();

    auto background = CCSprite::create("GJ_gradientBG.png");
    auto backgroundSize = background->getContentSize();

    background->setScaleX(size.width / backgroundSize.width);
    background->setScaleY(size.height / backgroundSize.height);
    background->setAnchorPoint({0, 0});
    background->setColor({0, 101, 253});
    background->setZOrder(-1);
    this->addChild(background);

    // Corners
    CCSprite* corner_left = CCSprite::createWithSpriteFrameName("GJ_sideArt_001.png");
    CCSprite* corner_right = CCSprite::createWithSpriteFrameName("GJ_sideArt_001.png");
    corner_right->setRotation(-90);
    corner_left->setPosition({35, 35});
    corner_right->setPosition({size.width - 35, 35});
    this->addChild(corner_left);
    this->addChild(corner_right);

    CCSprite* button = cocos2d::CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    auto backButton = CCMenuItemSpriteExtra::create(button, this, menu_selector(MoreLeaderboards::backButton));
    CCMenu* menu = cocos2d::CCMenu::create();
    menu->addChild(backButton);
    menu->setPosition({ 25, size.height - 25 });
    this->addChild(menu);

    if (type == "more") {
        // Change region
        auto menu_region = CCMenu::create();

        auto regionSpr = CCSprite::create("earth_btn.png"_spr);;
        regionSpr->setScale(.8f);
        auto regionBtn = CCMenuItemSpriteExtra::create(
            regionSpr,
            this,
            menu_selector(MoreLeaderboards::onRegion)
        );
        regionBtn->setPosition(239, -52);
        menu_region->addChild(regionBtn);

        auto infoSpr = cocos2d::CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
        auto infoBtn = CCMenuItemSpriteExtra::create(
            infoSpr,
            this,
            menu_selector(MoreLeaderboards::onInfo)
        );
        infoBtn->setPosition(-239, -120);
        menu_region->addChild(infoBtn);

        auto lbSpr = cocos2d::CCSprite::createWithSpriteFrameName("GJ_levelLeaderboardBtn_001.png");
        auto lbBtn = CCMenuItemSpriteExtra::create(
            lbSpr,
            this,
            menu_selector(MoreLeaderboards::onLBInfo)
        );
        lbBtn->setPosition(-239, -57);
        menu_region->addChild(lbBtn);

        this->addChild(menu_region);

        tab_page = 0;

        // tabs gradient
        #ifndef GEODE_IS_IOS
        m_tabsGradientNode = CCClippingNode::create();
        #else
        m_tabsGradientNode = CCNode::create();
        #endif
        m_tabsGradientNode->setContentSize(this->getContentSize());
        m_tabsGradientNode->setAnchorPoint({0.5f, 0.5f});
        m_tabsGradientNode->ignoreAnchorPointForPosition(true);
        m_tabsGradientNode->setZOrder(0);
        m_tabsGradientNode->setScale(0.8f);
        #ifndef GEODE_IS_IOS
        m_tabsGradientNode->setInverted(false);
        m_tabsGradientNode->setAlphaThreshold(0.7f);
        #endif

        m_tabsGradientSprite = CCSprite::create("tab-gradient.png"_spr);
        m_tabsGradientNode->addChild(m_tabsGradientSprite);

        m_tabsGradientStencil = CCSprite::create("tab-gradient-mask.png"_spr);
        m_tabsGradientStencil->setAnchorPoint({0.f, 0.f});
        m_tabsGradientStencil->setColor({172, 255, 67});
        m_tabsGradientStencil->setZOrder(1);
        m_tabsGradientStencil->setScale(0.8f);

        #ifndef GEODE_IS_IOS
        m_tabsGradientNode->setStencil(m_tabsGradientStencil);
        #endif

        this->addChild(m_tabsGradientNode);
        this->addChild(m_tabsGradientStencil);

        changeTabPage();

        // select first tab
        this->onTab(nullptr);
    } else if (type == "mods") {
        loading_circle = LoadingCircle::create();
        loading_circle->setZOrder(25);
        loading_circle->setParentLayer(this);
        loading_circle->show();
        
        startLoadingMods();
        loadPageMods();
    }
    setTouchEnabled(true);
    setKeypadEnabled(true);
    return true;
}

void MoreLeaderboards::backButton(cocos2d::CCObject*) {
    MoreLeaderboards::data_response_moreLB = "";
    MoreLeaderboards::g_tab = StatsListType::Stars;
    cocos2d::CCDirector::sharedDirector()->popSceneWithTransition(0.5F, cocos2d::PopTransition::kPopTransitionFade);
};

void MoreLeaderboards::keyBackClicked() {
    backButton(CCNode::create());
}

void MoreLeaderboards::fadeLoadingCircle() {
    if (loading_circle == nullptr) return;
    loading_circle->fadeAndRemove();
};

void MoreLeaderboards::handle_request_mods(std::string const& data) {
    if(!displayedData) { displayedData = CCArray::create(); displayedData->retain(); };
    if (data != "-1") {
        displayedData = CCArray::create();
        
        std::vector<std::string> mods = getWords(data, "|");

        while (mods.size() > 0) {
            std::string mod = mods[0];
            CCDictionary* modDict = CCDictionary::create();
            modDict->setObject(cocos2d::CCString::create(mod), "modstring");
            displayedData->addObject(modDict);
            mods.erase(mods.begin());
        };
    }
    loadPageMods();
}

void MoreLeaderboards::startLoadingMods() {
    const geode::utils::MiniFunction<void(std::string const&)> then = [this](std::string const& data) {
        handle_request_mods(data);
        fadeLoadingCircle();
    };
    const geode::utils::MiniFunction<void(std::string const&)> expect = [this](std::string const& error) {
        fadeLoadingCircle();
    };

    geode::utils::web::WebRequest request = web::WebRequest();
    RUNNING_REQUESTS.emplace(
        "@loaderModListCheck",
        request.get("https://clarifygdps.com/gdutils/modslist.php").map(
            [expect = std::move(expect), then = std::move(then)](web::WebResponse* response) {
                if (response->ok()) {
                    then(response->string().value());
                } else {
                    expect("An error occured while sending a request on our server. Please try again later.");
                }

                RUNNING_REQUESTS.erase("@loaderModListCheck");
                return *response;
            }
        )
    );
};

void MoreLeaderboards::loadPageMods() {
    if(listLayer != nullptr) listLayer->removeFromParentAndCleanup(true);

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    leaderboardView = MoreLeaderboardsListView::create(displayedData, 356.f, 220.f);
    listLayer = GJListLayer::create(leaderboardView, "GD Moderators", {191, 114, 62, 255}, 356.f, 220.f, 0);
    listLayer->setPosition(winSize / 2 - listLayer->getScaledContentSize() / 2 - CCPoint(0,5));

    addChild(listLayer);
}

void MoreLeaderboards::startLoadingMore() {
    geode::utils::web::WebRequest request = web::WebRequest();

    this->retain();

    const geode::utils::MiniFunction<void(std::string const&)> expect = [this](std::string const& error) {
        loading = false;
        auto scene = CCDirector::sharedDirector()->getRunningScene();
        auto layer = scene->getChildren()->objectAtIndex(0);
        if (layer == nullptr) return this->release();
        if (misc::getNodeName(layer) != "MoreLeaderboards") return this->release();
        geode::createQuickPopup(
            "Error",
            "An error occured while sending a request on <cy>our server</c>. Please try again later.",
            "OK", nullptr,
            [this](auto, bool btn2) {
                if (!btn2) {
                    g_tab = StatsListType::Diamonds;
                    keyBackClicked();
                }
            }
        );
        fadeLoadingCircle();
        loading = false;
        this->release();
    };

    const geode::utils::MiniFunction<void(std::string const&)> then = [this](std::string const& data) {
        if (data != "-1") {
            data_region = data;
            SelectRegion::displayedData = MoreLeaderboards::getWords(data, "|");
        }

        std::string type = "";
        geode::utils::web::WebRequest request = web::WebRequest();

        switch (g_tab) {
            case StatsListType::Diamonds:
                type = "diamonds";
                break;
            case StatsListType::UserCoins:
                type = "ucoins";
                break;
            case StatsListType::Demons:
                type = "demons";
                break;
            case StatsListType::Moons:
                type = "moons";
                break;
            case StatsListType::Creators:
                type = "cp";
                break;
            case StatsListType::Stars:
                type = "stars";
                break;
            case StatsListType::classicDemonsEasy:
                type = "classicDemonsEasy";
                break;
            case StatsListType::classicDemonsMedium:
                type = "classicDemonsMedium";
                break;
            case StatsListType::classicDemonsHard:
                type = "classicDemonsHard";
                break;
            case StatsListType::classicDemonsInsane:
                type = "classicDemonsInsane";
                break;
            case StatsListType::classicDemonsExtreme:
                type = "classicDemonsExtreme";
                break;
            case StatsListType::platformerDemonsEasy:
                type = "platformerDemonsEasy";
                break;
            case StatsListType::platformerDemonsMedium:
                type = "platformerDemonsMedium";
                break;
            case StatsListType::platformerDemonsHard:
                type = "platformerDemonsHard";
                break;
            case StatsListType::platformerDemonsInsane:
                type = "platformerDemonsInsane";
                break;
            case StatsListType::platformerDemonsExtreme:
                type = "platformerDemonsExtreme";
                break;
            case StatsListType::BetterProgression:
                type = "betterProgression";
                break;
        }

        const geode::utils::MiniFunction<void(std::string const&)> expect = [this](std::string const& error) {
            loading = false;
            auto scene = CCDirector::sharedDirector()->getRunningScene();
            auto layer = scene->getChildren()->objectAtIndex(0);
            if (layer == nullptr) return this->release();
            if (misc::getNodeName(layer) != "MoreLeaderboards") return this->release();
            geode::createQuickPopup(
                "Error",
                "An error occured while sending a request on <cy>our server</c>. Please try again later.",
                "OK", nullptr,
                [this](auto, bool btn2) {
                    if (!btn2) {
                        g_tab = StatsListType::Diamonds;
                        keyBackClicked();
                    }
                }
            );
            fadeLoadingCircle();
            loading = false;
            this->release();
        };

        const geode::utils::MiniFunction<void(std::string const&)> then = [this](std::string const& data) {
            loading = false;
            auto scene = CCDirector::sharedDirector()->getRunningScene();
            auto layer = scene->getChildren()->objectAtIndex(0);
            if (layer == nullptr) return this->release();
            if (misc::getNodeName(layer) != "MoreLeaderboards") return this->release(); // prevent le crash, even though the layer shouldve already been destructed
            if (data == "-1" || data.length() < 2) {
                fadeLoadingCircle();
                geode::createQuickPopup(
                    "Error",
                    "An error occured while sending a request on <cy>our server</c>. Please try again later.",
                    "OK", nullptr,
                    [this](auto, bool btn2) {
                        if (!btn2) {
                            g_tab = StatsListType::Diamonds;
                            keyBackClicked();
                        }
                    }
                );
                this->release();
                return;
            }

            handle_request_more(data);
            fadeLoadingCircle();
            loading = false;

            this->release();
        };

        RUNNING_REQUESTS.emplace(
            "@loaderMoreLeaderboardCheck",
            request.param("type", type).param("page", page).param("country", country_id).get("https://clarifygdps.com/gdutils/moreleaderboards.php").map(
                [expect = std::move(expect), then = std::move(then)](web::WebResponse* response) {
                    if (response->ok()) {
                        then(response->string().value());
                    } else {
                        expect("An error occured while sending a request on our server. Please try again later.");
                    }

                    RUNNING_REQUESTS.erase("@loaderMoreLeaderboardCheck");
                    return *response;
                }
            )
        );
    };

    if (data_region == "") {
        RUNNING_REQUESTS.emplace(
        "@loaderMoreLeaderboardRegionGet",
        request.get("https://clarifygdps.com/gdutils/moreleaderboards_region.php").map(
            [expect = std::move(expect), then = std::move(then)](web::WebResponse* response) {
                if (response->ok()) {
                    then(response->string().value());
                } else {
                    expect("An error occured while sending a request on our server. Please try again later.");
                }

                RUNNING_REQUESTS.erase("@loaderMoreLeaderboardRegionGet");
                return *response;
            }
        ));
    } else {
        then(data_region);
    }
};

void MoreLeaderboards::handle_request_more(std::string const& data) {
    if(!displayedData) { displayedData = CCArray::create(); displayedData->retain(); };

    if (data != "-1") {
        data_response_moreLB = data;

        displayedData = CCArray::create();
        std::vector<std::string> dataString = getWords(data, "#");
        std::vector<std::string> users = getWords(dataString[0], "|");
        std::vector<std::string> data_page = getWords(dataString[1], "|");

        while (users.size() > 0) {
            std::string user = users[0];

            auto score = GJUserScore::create(
                MoreLeaderboards::responseToDict(user)
            );
            displayedData->addObject(score);
            users.erase(users.begin());
        };

        int id = 0;
        while (data_page.size() > 0) {
            std::string page_test = data_page[0];

            if (id == 0) {
                start_count = std::stoi(page_test);
            } else if (id == 1) {
                end_count = std::stoi(page_test);
            } else if (id == 2) {
                total_count = std::stoi(page_test);
            } else if (id == 3) {
                page = std::stoi(page_test);
            }

            id++;
            data_page.erase(data_page.begin());
        }
    }

    loadPageMore();
    loadPageStats();
}

void MoreLeaderboards::loadPageMore() {
    if(listLayer != nullptr) listLayer->removeFromParentAndCleanup(true);

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    leaderboardViewScore = CustomListView::create(displayedData, nullptr, 220.f, 356.f, 0, BoomListType::Score, 0.0F);
    listLayer = GJListLayer::create(leaderboardViewScore, nullptr, {191, 114, 62, 255}, 356.f, 220.f, 0);
    listLayer->setZOrder(31);
    listLayer->setPosition(winSize / 2 - listLayer->getScaledContentSize() / 2 - CCPoint(0,5));

    addChild(listLayer);
}

void MoreLeaderboards::onSearch(CCObject*) {
    SearchUserLBLayer::create(this)->show();
}

void MoreLeaderboards::loadPageStats() {
    auto scene = CCDirector::sharedDirector()->getRunningScene();
    auto winSize = CCDirector::sharedDirector()->getWinSize();

    if (page_label != nullptr) page_label->removeFromParentAndCleanup(true);

    CCMenu* menu_label = CCMenu::create();
    menu_label->setLayout(
        RowLayout::create()
        ->setAxisAlignment(AxisAlignment::Center)
        ->setGap(10.f)
    );
    menu_label->setPosition({ winSize.width / 2, 12.f });
    menu_label->setContentSize({ 300.f, 7.f });

    std::string fmt = fmt::format("Top {} - {} of {}", start_count, end_count, total_count);
    page_label = CCLabelBMFont::create(fmt.c_str(), "goldFont.fnt");
    page_label->setZOrder(40);
    page_label->setLayoutOptions(
        AxisLayoutOptions::create()
        ->setAutoScale(true)
        ->setMinScale(0)
        ->setMaxScale(0.7f)
        ->setScalePriority(1)
    );
    menu_label->addChild(page_label);

    trophy = CCSprite::createWithSpriteFrameName("rankIcon_top10_001.png");
    trophy->setScale(.3f);
    trophy->setPosition(CCPoint { -5.f, 0.f } - page_label->getScaledContentSize().width);
    menu_label->addChild(trophy);

    addChild(menu_label);
    menu_label->updateLayout();

    CCMenu* menu = CCMenu::create();

    if (page_left != nullptr) page_left->removeFromParentAndCleanup(true);
    if (page_right != nullptr) page_right->removeFromParentAndCleanup(true);

    if (page > 0) {
        page_left = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png"),
            this,
            menu_selector(MoreLeaderboards::onPageLeft)
        );
        page_left->setPosition(-220, 0);
        menu->addChild(page_left);
    }

    if (end_count < total_count) {
        auto sprite = CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png");
        sprite->setFlipX(true);
        page_right = CCMenuItemSpriteExtra::create(
            sprite,
            this,
            menu_selector(MoreLeaderboards::onPageRight)
        );
        page_right->setPosition(220, 0);
        menu->addChild(page_right);
    }

    addChild(menu);

    loadTabPageButtons();
}

void MoreLeaderboards::loadTabPageButtons() {
    if (tab_page > 0) {
        auto tab_page_left_sprite = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
        tab_page_left_sprite->setScale(0.6f);
        tab_page_left = CCMenuItemSpriteExtra::create(
            tab_page_left_sprite,
            this,
            menu_selector(MoreLeaderboards::onTabPageLeft)
        );
        tab_page_left->setPosition(-184, 138);
        m_menu->addChild(tab_page_left);
    }
    
    if (tab_page < 2) {
        auto tab_page_right_sprite = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
        tab_page_right_sprite->setScale(0.6f);
        tab_page_right_sprite->setFlipX(true);
        tab_page_right = CCMenuItemSpriteExtra::create(
            tab_page_right_sprite,
            this,
            menu_selector(MoreLeaderboards::onTabPageRight)
        );
        tab_page_right->setPosition(184, 138);
        m_menu->addChild(tab_page_right);
    }

    auto searchSpr = CCSprite::createWithSpriteFrameName("gj_findBtn_001.png");
    m_search = CCMenuItemSpriteExtra::create(
        searchSpr,
        this,
        menu_selector(MoreLeaderboards::onSearch)
    );
    m_search->setPosition(239, 104);
    m_menu->addChild(m_search);
}

void MoreLeaderboards::onTabPageLeft(CCObject* pSender) {
    if (loading) return;

    tab_page--;
    changeTabPage();

    page = 0;
    this->onTab(nullptr);
}

void MoreLeaderboards::onTabPageRight(CCObject* pSender) {
    if (loading) return;

    tab_page++;
    changeTabPage();

    page = 0;
    this->onTab(nullptr);
}

void MoreLeaderboards::changeTabPage() {
    if (m_tab1 != nullptr) m_tab1->removeFromParentAndCleanup(true);
    if (m_tab2 != nullptr) m_tab2->removeFromParentAndCleanup(true);
    if (m_tab3 != nullptr) m_tab3->removeFromParentAndCleanup(true);
    if (m_tab4 != nullptr) m_tab4->removeFromParentAndCleanup(true);
    if (m_tab5 != nullptr) m_tab5->removeFromParentAndCleanup(true);
    if (m_tab6 != nullptr) m_tab6->removeFromParentAndCleanup(true);

    m_tab1 = nullptr;
    m_tab2 = nullptr;
    m_tab3 = nullptr;
    m_tab4 = nullptr;
    m_tab5 = nullptr;
    m_tab6 = nullptr;

    if (m_menu != nullptr) m_menu->removeFromParentAndCleanup(true);

    m_menu = CCMenu::create();
    m_menu->setZOrder(1);

    auto stars_sprite = CCSprite::createWithSpriteFrameName("star_small01_001.png");
    auto moons_sprite = CCSprite::createWithSpriteFrameName("moon_small01_001.png");
    auto diamond_sprite = CCSprite::createWithSpriteFrameName("diamond_small01_001.png");
    auto usercoins_sprite = CCSprite::createWithSpriteFrameName("GJ_coinsIcon2_001.png");
    auto demons_sprite = CCSprite::createWithSpriteFrameName("GJ_demonIcon_001.png");
    auto creators_sprite = CCSprite::createWithSpriteFrameName("GJ_hammerIcon_001.png");

    auto classic_sprite = CCSprite::createWithSpriteFrameName("star_small01_001.png");
    classic_sprite->setPositionX(30);
    classic_sprite->setScale(1.2f);

    auto platformer_sprite = CCSprite::createWithSpriteFrameName("moon_small01_001.png");
    platformer_sprite->setPositionX(30);
    platformer_sprite->setScale(1.2f);

    auto better_progression = CCSprite::create("better_progression.png"_spr);

    auto easydemon_sprite_classic = CCSprite::createWithSpriteFrameName("diffIcon_07_btn_001.png");
    easydemon_sprite_classic->addChild(classic_sprite);
    auto mediumdemon_sprite_classic = CCSprite::createWithSpriteFrameName("diffIcon_08_btn_001.png");
    mediumdemon_sprite_classic->addChild(classic_sprite);
    auto harddemon_sprite_classic = CCSprite::createWithSpriteFrameName("diffIcon_06_btn_001.png");
    harddemon_sprite_classic->addChild(classic_sprite);
    auto insane_sprite_classic = CCSprite::createWithSpriteFrameName("diffIcon_09_btn_001.png");
    insane_sprite_classic->addChild(classic_sprite);
    auto extreme_sprite_classic = CCSprite::createWithSpriteFrameName("diffIcon_10_btn_001.png");
    extreme_sprite_classic->addChild(classic_sprite);

    auto easydemon_sprite_platformer = CCSprite::createWithSpriteFrameName("diffIcon_07_btn_001.png");
    easydemon_sprite_platformer->addChild(platformer_sprite);
    auto mediumdemon_sprite_platformer = CCSprite::createWithSpriteFrameName("diffIcon_08_btn_001.png");
    mediumdemon_sprite_platformer->addChild(platformer_sprite);
    auto harddemon_sprite_platformer = CCSprite::createWithSpriteFrameName("diffIcon_06_btn_001.png");
    harddemon_sprite_platformer->addChild(platformer_sprite);
    auto insane_sprite_platformer = CCSprite::createWithSpriteFrameName("diffIcon_09_btn_001.png");
    insane_sprite_platformer->addChild(platformer_sprite);
    auto extreme_sprite_platformer = CCSprite::createWithSpriteFrameName("diffIcon_10_btn_001.png");
    extreme_sprite_platformer->addChild(platformer_sprite);

    switch (tab_page) {
        case 0:
            g_tab = StatsListType::Stars;

            m_tab1 = NewTabButton::create(TabBaseColor::Unselected, TabBaseColor::Selected, stars_sprite, this, menu_selector(MoreLeaderboards::onTab));
            m_tab1->setPosition(-140.f, 132);
            m_tab1->setTag(static_cast<int>(StatsListType::Stars));
            m_tab1->setZOrder(30);
            m_tab1->setScale(0.8f);
            m_menu->addChild(m_tab1);

            m_tab2 = NewTabButton::create(TabBaseColor::Unselected, TabBaseColor::Selected, moons_sprite, this, menu_selector(MoreLeaderboards::onTab));
            m_tab2->setPosition(-86.f, 132);
            m_tab2->setTag(static_cast<int>(StatsListType::Moons));
            m_tab2->setZOrder(30);
            m_tab2->setScale(0.8f);
            m_menu->addChild(m_tab2);
            
            m_tab3 = NewTabButton::create(TabBaseColor::Unselected, TabBaseColor::Selected, usercoins_sprite, this, menu_selector(MoreLeaderboards::onTab));
            m_tab3->setPosition(-30.f, 132);
            m_tab3->setTag(static_cast<int>(StatsListType::UserCoins));
            m_tab3->setZOrder(30);
            m_tab3->setScale(0.8f);
            m_menu->addChild(m_tab3);
            
            m_tab4 = NewTabButton::create(TabBaseColor::Unselected, TabBaseColor::Selected, demons_sprite, this, menu_selector(MoreLeaderboards::onTab));
            m_tab4->setPosition(26.f, 132);
            m_tab4->setTag(static_cast<int>(StatsListType::Demons));
            m_tab4->setZOrder(30);
            m_tab4->setScale(0.8f);
            m_menu->addChild(m_tab4);

            m_tab5 = NewTabButton::create(TabBaseColor::Unselected, TabBaseColor::Selected, diamond_sprite, this, menu_selector(MoreLeaderboards::onTab));
            m_tab5->setPosition(82.f, 132);
            m_tab5->setTag(static_cast<int>(StatsListType::Diamonds));
            m_tab5->setZOrder(30);
            m_tab5->setScale(0.8f);
            m_menu->addChild(m_tab5);

            m_tab6 = NewTabButton::create(TabBaseColor::Unselected, TabBaseColor::Selected, creators_sprite, this, menu_selector(MoreLeaderboards::onTab));
            m_tab6->setPosition(138.f, 132);
            m_tab6->setTag(static_cast<int>(StatsListType::Creators));
            m_tab6->setZOrder(30);
            m_tab6->setScale(0.8f);
            m_menu->addChild(m_tab6);

            break;
        case 1:
            g_tab = StatsListType::BetterProgression;

            m_tab1 = NewTabButton::create(TabBaseColor::Unselected, TabBaseColor::Selected, better_progression, this, menu_selector(MoreLeaderboards::onTab));
            m_tab1->setPosition(-140.f, 132);
            m_tab1->setTag(static_cast<int>(StatsListType::BetterProgression));
            m_tab1->setZOrder(30);
            m_tab1->setScale(0.8f);
            m_menu->addChild(m_tab1);

            m_tab2 = NewTabButton::create(TabBaseColor::Unselected, TabBaseColor::Selected, easydemon_sprite_classic, this, menu_selector(MoreLeaderboards::onTab));
            m_tab2->setPosition(-86.f, 132);
            m_tab2->setTag(static_cast<int>(StatsListType::classicDemonsEasy));
            m_tab2->setZOrder(30);
            m_tab2->setScale(0.8f);
            m_menu->addChild(m_tab2);

            m_tab3 = NewTabButton::create(TabBaseColor::Unselected, TabBaseColor::Selected, mediumdemon_sprite_classic, this, menu_selector(MoreLeaderboards::onTab));
            m_tab3->setPosition(-30.f, 132);
            m_tab3->setTag(static_cast<int>(StatsListType::classicDemonsMedium));
            m_tab3->setZOrder(30);
            m_tab3->setScale(0.8f);
            m_menu->addChild(m_tab3);

            m_tab4 = NewTabButton::create(TabBaseColor::Unselected, TabBaseColor::Selected, harddemon_sprite_classic, this, menu_selector(MoreLeaderboards::onTab));
            m_tab4->setPosition(26.f, 132);
            m_tab4->setTag(static_cast<int>(StatsListType::classicDemonsHard));
            m_tab4->setZOrder(30);
            m_tab4->setScale(0.8f);
            m_menu->addChild(m_tab4);

            m_tab5 = NewTabButton::create(TabBaseColor::Unselected, TabBaseColor::Selected, insane_sprite_classic, this, menu_selector(MoreLeaderboards::onTab));
            m_tab5->setPosition(82.f, 132);
            m_tab5->setTag(static_cast<int>(StatsListType::classicDemonsInsane));
            m_tab5->setZOrder(30);
            m_tab5->setScale(0.8f);
            m_menu->addChild(m_tab5);

            m_tab6 = NewTabButton::create(TabBaseColor::Unselected, TabBaseColor::Selected, extreme_sprite_classic, this, menu_selector(MoreLeaderboards::onTab));
            m_tab6->setPosition(138.f, 132);
            m_tab6->setTag(static_cast<int>(StatsListType::classicDemonsExtreme));
            m_tab6->setZOrder(30);
            m_tab6->setScale(0.8f);
            m_menu->addChild(m_tab6);

            break;
        case 2:
            g_tab = StatsListType::platformerDemonsEasy;

            m_tab1 = NewTabButton::create(TabBaseColor::Unselected, TabBaseColor::Selected, easydemon_sprite_platformer, this, menu_selector(MoreLeaderboards::onTab));
            m_tab1->setPosition(-140.f, 132);
            m_tab1->setTag(static_cast<int>(StatsListType::platformerDemonsEasy));
            m_tab1->setZOrder(30);
            m_tab1->setScale(0.8f);
            m_menu->addChild(m_tab1);

            m_tab2 = NewTabButton::create(TabBaseColor::Unselected, TabBaseColor::Selected, mediumdemon_sprite_platformer, this, menu_selector(MoreLeaderboards::onTab));
            m_tab2->setPosition(-86.f, 132);
            m_tab2->setTag(static_cast<int>(StatsListType::platformerDemonsMedium));
            m_tab2->setZOrder(30);
            m_tab2->setScale(0.8f);
            m_menu->addChild(m_tab2);

            m_tab3 = NewTabButton::create(TabBaseColor::Unselected, TabBaseColor::Selected, harddemon_sprite_platformer, this, menu_selector(MoreLeaderboards::onTab));
            m_tab3->setPosition(-30.f, 132);
            m_tab3->setTag(static_cast<int>(StatsListType::platformerDemonsHard));
            m_tab3->setZOrder(30);
            m_tab3->setScale(0.8f);
            m_menu->addChild(m_tab3);

            m_tab4 = NewTabButton::create(TabBaseColor::Unselected, TabBaseColor::Selected, insane_sprite_platformer, this, menu_selector(MoreLeaderboards::onTab));
            m_tab4->setPosition(26.f, 132);
            m_tab4->setTag(static_cast<int>(StatsListType::platformerDemonsInsane));
            m_tab4->setZOrder(30);
            m_tab4->setScale(0.8f);
            m_menu->addChild(m_tab4);

            m_tab5 = NewTabButton::create(TabBaseColor::Unselected, TabBaseColor::Selected, extreme_sprite_platformer, this, menu_selector(MoreLeaderboards::onTab));
            m_tab5->setPosition(82.f, 132);
            m_tab5->setTag(static_cast<int>(StatsListType::platformerDemonsExtreme));
            m_tab5->setZOrder(30);
            m_tab5->setScale(0.8f);
            m_menu->addChild(m_tab5);

            break;
    }

    this->addChild(m_menu);
}

void MoreLeaderboards::onPageLeft(CCObject* pSender) {
    if (loading) return;

    loading = true;

    loading_circle = LoadingCircle::create();
    loading_circle->setParentLayer(this);
    loading_circle->setZOrder(25);
    loading_circle->show();

    if (displayedData) {
        displayedData->release();
        displayedData = cocos2d::CCArray::create();
        displayedData->retain();
    }

    page--;
    resetInfos();
    
    if (loading) {
        startLoadingMore();
        loadPageMore();
    }
}

void MoreLeaderboards::onPageRight(CCObject* pSender) {
    if (loading) return;

    loading = true;

    loading_circle = LoadingCircle::create();
    loading_circle->setZOrder(25);
    loading_circle->setParentLayer(this);
    loading_circle->show();

    if (displayedData) {
        displayedData->release();
        displayedData = cocos2d::CCArray::create();
        displayedData->retain();
    }

    page++;
    resetInfos();
    
    if (loading) {
        startLoadingMore();
        loadPageMore();
    }
}

void MoreLeaderboards::resetInfos() {
    if (page_label != nullptr) page_label->removeFromParentAndCleanup(true);
    if (page_left != nullptr) page_left->removeFromParentAndCleanup(true);
    if (page_right != nullptr) page_right->removeFromParentAndCleanup(true);
    if (trophy != nullptr) trophy->removeFromParentAndCleanup(true);

    page_label = nullptr;
    page_left = nullptr;
    page_right = nullptr;
    trophy = nullptr;
}

void MoreLeaderboards::onTab(CCObject* pSender) {
    if (loading) return;

    loading = true;

    loading_circle = LoadingCircle::create();
    loading_circle->setZOrder(25);
    loading_circle->setParentLayer(this);
    loading_circle->show();

    if (displayedData) {
        displayedData->release();
        displayedData = cocos2d::CCArray::create();
        displayedData->retain();
    }

    if (pSender) {
        g_tab = static_cast<StatsListType>(pSender->getTag());
    }

    resetInfos();

    page = 0;

    auto toggleTab = [this](CCMenuItemToggler* member) -> void {
        auto isSelected = member->getTag() == static_cast<int>(g_tab);
        auto targetMenu = m_menu;
        member->toggle(isSelected);
        if (member->getParent() != targetMenu) {
            member->retain();
            member->removeFromParent();
            targetMenu->addChild(member);
            member->release();
        }
        if (isSelected && m_tabsGradientStencil)
            m_tabsGradientStencil->setPosition(member->m_onButton->convertToWorldSpace({0.f, 0.f}));
    };

    if (m_tab1 != nullptr) toggleTab(m_tab1);
    if (m_tab2 != nullptr) toggleTab(m_tab2);
    if (m_tab3 != nullptr) toggleTab(m_tab3);
    if (m_tab4 != nullptr) toggleTab(m_tab4);
    if (m_tab5 != nullptr) toggleTab(m_tab5);
    if (m_tab6 != nullptr) toggleTab(m_tab6);

    if (loading) {
        startLoadingMore();
        loadPageMore();
    }
}

class $modify(LeaderboardsLayer) {
    bool init(LeaderboardState state) {
        if (!LeaderboardsLayer::init(state)) return false;

        auto menu = CCMenu::create();

        auto plusSpr = CCSprite::createWithSpriteFrameName("GJ_plusBtn_001.png");
        plusSpr->setScale(.8f);
        auto plusBtn = CCMenuItemSpriteExtra::create(
            plusSpr,
            this,
            menu_selector(MoreLeaderboards::onMoreLeaderboards)
        );
        plusBtn->setPosition(239, 20);
        menu->addChild(plusBtn);

        auto modsSpr = CCSprite::createWithSpriteFrameName("modBadge_01_001.png");
        modsSpr->setScale(1.5f);
        auto modsBtn = CCMenuItemSpriteExtra::create(
            modsSpr,
            this,
            menu_selector(MoreLeaderboards::onModsList)
        );
        modsBtn->setPosition(239, -24);
        menu->addChild(modsBtn);

        this->addChild(menu);

        return true;
    }
};