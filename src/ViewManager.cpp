#include "ViewManager.h"

namespace tjg {
    ViewManager::ViewManager(LogicCenter &logic_center, ResourceManager &resource_manager, EventManager &event_manager, std::shared_ptr<SoundManager> &sound_manager):
            logic_center(logic_center),
            event_manager(event_manager),
            state(State::MAIN_MENU),
            window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, 32), "Serene", sf::Style::Titlebar | sf::Style::Close),
            main_menu_view(window, resource_manager, sound_manager),
            level_menu_view(window, resource_manager, sound_manager),
            level_view(window, resource_manager, sound_manager, logic_center),
            pause_menu_view(window, resource_manager, sound_manager),
            win_menu_view(window, resource_manager, sound_manager),
            finish_menu_view(window, resource_manager, sound_manager),
            fail_menu_view(window, resource_manager, sound_manager) {
        window.setVerticalSyncEnabled(true);
    }

    void ViewManager::Initialize() {
        unlocked = ReadUnlockedLevel();
    }

    bool ViewManager::Running(){
        return running;
    }

    void ViewManager::SwitchView(ViewSwitch view_switch) {
        event_manager.Fire<ViewChanged>(view_switch);
        switch (view_switch.state) {
            case State::MAIN_MENU:
                SwitchToMainMenuView();
                break;
            case State::LEVEL_MENU:
                SwitchToLevelMenuView(unlocked);
                break;
            case State::PLAYING:
                logic_center.Reset();
                if (view_switch.level_number > 0) {
                    current_level = view_switch.level_number;
                    SwitchToLevelView(view_switch.level_number);
                } else {
                    SwitchToLevelView(current_level);
                }
                break;
            case State::RESUME:
                ResumePlayerView();
                break;
            case State::PAUSED:
                SwitchToPauseMenuView();
                break;
            case State::WON:
                SwitchToWinMenuView();
                break;
            case State::FAILED:
                SwitchToFailMenuView();
                break;
            case State::EXIT:
                window.close();
                running = false;
                break;
            default:
                break;
        }
    }

    void ViewManager::SwitchToMainMenuView() {
        main_menu_view.Initialize();
        state = State::MAIN_MENU;
    }

    void ViewManager::SwitchToLevelMenuView(unsigned int unlocked) {
        level_menu_view.Initialize(unlocked);
        state = State::LEVEL_MENU;
    }

    void ViewManager::SwitchToPauseMenuView() {
        pause_menu_view.Initialize(current_level);
        this->state = State::PAUSED;
    }

    void ViewManager::SwitchToWinMenuView() {
        if (current_level >= LEVEL_MENU_OPTIONS) {
            SwitchToFinishMenuView();
            return;
        }
        if (current_level + 1 > unlocked) unlocked = current_level + 1;
        WriteUnlockedLevel(unlocked);
        win_menu_view.Initialize(current_level);
        this->state = State::WON;
    }

    void ViewManager::SwitchToFinishMenuView() {
        finish_menu_view.Initialize();
        this->state = State::FINISH;
    }

    void ViewManager::SwitchToFailMenuView() {
        fail_menu_view.Initialize(current_level);
        this->state = State::FAILED;
    }

    void ViewManager::SwitchToLevelView(unsigned int level_number) {
        logic_center.Initialize(level_number);
        level_view.Initialize();
        state = State::PLAYING;
    }

    void ViewManager::ResumePlayerView() {
        state = State::PLAYING;
    }

    void ViewManager::Update(sf::Time elapsed){
        switch (state) {
            case State::MAIN_MENU:
                HandleWindowEvents(main_menu_view);
                if (state == State::MAIN_MENU) {
                    main_menu_view.Update();
                }
                break;
            case State::LEVEL_MENU:
                HandleWindowEvents(level_menu_view);
                if (state == State::LEVEL_MENU) {
                    level_menu_view.Update();
                }
                break;
            case State::WON:
                HandleWindowEvents(win_menu_view);
                win_menu_view.Update();
                break;
            case State::FINISH:
                HandleWindowEvents(finish_menu_view);
                finish_menu_view.Update();
                break;
            case State::FAILED:
                HandleWindowEvents(fail_menu_view);
                fail_menu_view.Update();
                break;
            case State::PAUSED:
                HandleWindowEvents(pause_menu_view);
                pause_menu_view.Update();
                break;
            case State::PLAYING:
                HandleWindowEvents(level_view);
                if (state == State::PLAYING) {
                    logic_center.Update(elapsed);
                    level_view.Update(elapsed);
                }
                break;
            default:
                break;
        }

        //Switch to menu if won/lost
        switch (logic_center.GetGameState()) {
            case State::WON:
                logic_center.Reset();
                SwitchToWinMenuView();
                break;
            case State::FAILED:
                logic_center.Reset();
                SwitchToFailMenuView();
                break;
            default:
                break;
        }
    }

    void ViewManager::Render(){
        switch (state) {
            case State::MAIN_MENU:
                main_menu_view.Render();
                break;
            case State::LEVEL_MENU:
                level_menu_view.Render();
                break;
            case State::PAUSED:
                pause_menu_view.Render();
                break;
            case State::WON:
                win_menu_view.Render();
                break;
            case State::FINISH:
                finish_menu_view.Render();
                break;
            case State::FAILED:
                fail_menu_view.Render();
                break;
            case State::PLAYING:
                level_view.Render();
                break;
            default:
                break;
        }
    }

    void ViewManager::HandleWindowEvents(View &current_view) {
        sf::Event event;
        // Look for window events.
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                    window.close();
                    running = false;
                    break;
                default:
                    auto view_switch = current_view.HandleWindowEvents(event);
                    if (view_switch.state != State::CONTINUE) {
                        SwitchView(view_switch);
                    }
                    break;
            }
        }
    }

    unsigned int ViewManager::ReadUnlockedLevel() {
        unsigned int unlocked = 1;
        std::ifstream progress_file ("../data/progress.log");
        if (progress_file.is_open()) {
            std::cout << "Reading progress: ";
            progress_file >> unlocked;
            std::cout << "Unlocked Level " << std::to_string(unlocked) << std::endl;
            progress_file.close();
        } else {
            std::cout << "Error reading progress. Starting new game." <<std::endl;
        }
        if (unlocked > LEVEL_MENU_OPTIONS) unlocked = LEVEL_MENU_OPTIONS;
        return unlocked;
    }

    void ViewManager::WriteUnlockedLevel(unsigned int level_number) {
        if (level_number > LEVEL_MENU_OPTIONS) level_number = LEVEL_MENU_OPTIONS;
        std::ofstream progress_file ("../data/progress.log");
        if (progress_file.is_open()) {
            std::cout << "Saving progress: Unlocked Level " << std::to_string(level_number) << std::endl;
            progress_file << std::to_string (level_number);
            progress_file.close();
        } else {
            std::cout << "Error saving progress." << std::endl;
        }

    }
}
