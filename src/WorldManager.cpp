#include "WorldManager.h"

WorldManager::WorldManager(std::string path_to_def_texture, int physics_quality, int moving_inertia_frames, float move_speed, float zoom_speed)
{
    WorldManager::path_to_def_texture = path_to_def_texture;
    WorldManager::physics_quality = physics_quality;
    WorldManager::moving_inertia_frames = moving_inertia_frames;
    WorldManager::move_speed = move_speed;
    WorldManager::zoom_speed = zoom_speed;

    b2Vec2 gravity = b2Vec2(0.0f, 9.81f);
    WorldManager::world = new b2World(gravity);

    WorldManager::objects = std::vector<BasePObj*>();

    Font::LoadFont("./assets/fonts/PressStart2P-vaV7.ttf");
}
WorldManager::~WorldManager()
{
    /// @brief Delete all the objects.
    for (int i = WorldManager::objects.size() - 1; i >= 0; i--)
    {
        WorldManager::DeleteObject(i);
    }
    /// @brief Unload all the textures.
    for (std::map<std::string, SDL_Texture*>::iterator itr = WorldManager::textures.begin(); itr != WorldManager::textures.end(); itr++)
    {
        SDL_DestroyTexture(itr->second);
    }
    /// @brief Destroy the world.
    delete WorldManager::world;
}

void WorldManager::LoadLevel(Level level, Renderer* renderer)
{
    WorldManager::level = level;

    WorldManager::textures[""] = SDL_CreateTextureFromSurface(renderer->GetRenderer(), IMG_Load(WorldManager::path_to_def_texture.c_str()));

    // CAMERA
    auto camera = WorldManager::level.GetCamera();

    if (camera.type == "static")
    {
        WorldManager::zoom = renderer->GetHeight() / camera.height;

        WorldManager::x_offset =    -(camera.x * WorldManager::zoom)
                                    +(renderer->GetWidth() / 2);

        WorldManager::y_offset =    -(camera.y * WorldManager::zoom)
                                    +(renderer->GetHeight() / 2);
    }
    /////////
    
    // OBJECTS
    for (int i = WorldManager::objects.size() - 1; i >= 0; i--)
    { // Remove current loaded objects
        WorldManager::DeleteObject(i);
    }
    
    auto objects = WorldManager::level.GetPObjects();
    for (size_t i = 0; i < objects.size(); i++)
    {
        WorldManager::AddObject(objects[i]);
    }
    //////////

    // CYCLES (everything other at the end of the Step())
    WorldManager::cyclesDelays = std::vector<int>();

    for (size_t i = 0; i < WorldManager::level.GetCycles().size(); i++)
    {
        WorldManager::cyclesDelays.push_back(1);
    }    
    /////////

    // ACTIONS (everything other at control handling sector)
    WorldManager::actions = WorldManager::level.GetActions();
    //////////
}

SDL_Texture* WorldManager::LoadTexture(std::string path, SDL_Renderer* renderer)
{
    if (WorldManager::textures.count(path) == 0)
    {
        auto texture = SDL_CreateTextureFromSurface(renderer, IMG_Load(path.c_str()));
        WorldManager::textures[path] = texture != NULL ? texture : nullptr;
    }
    return WorldManager::textures[path];
}

void WorldManager::AddObject(BasePObj* obj)
{
    WorldManager::order.push_back(obj);
}
void WorldManager::DeleteObject(int index)
{
    delete WorldManager::objects[index]; // Call a destructor.
    WorldManager::objects.erase(WorldManager::objects.begin() + index); // Remove from vector.
}

template<typename T>
double GetAverage(std::vector<T> const& v)
{
    if (v.empty())
        return 0;
 
    return std::reduce(v.begin(), v.end(), 0.0) / v.size();
}

// Used for decreasing actions code
void HandleActionCtrl(bool old, bool now, Json::Value key, Level level, std::vector<BasePObj*> objects)
{
    if (now && !old)
        level.PerformAction(key["keydown_once"], objects);
    else if (now)
        level.PerformAction(key["keydown_hold"], objects);
    else if (!now && old)
        level.PerformAction(key["keyup"], objects);
}

std::vector<int> last_frames_speed_x = std::vector<int>();
std::vector<int> last_frames_speed_y = std::vector<int>();

void WorldManager::Step(Renderer* renderer, Controls ctrl, Controls old_ctrl)
{
    if (ctrl.Debug() && !old_ctrl.Debug())
    {
        WorldManager::isDebug = !WorldManager::isDebug;
    }        

    WorldManager::y_offset += ctrl.MoveUp() * WorldManager::move_speed;
    WorldManager::x_offset -= ctrl.MoveRight() * WorldManager::move_speed;
    WorldManager::y_offset -= ctrl.GetMoveDown() * WorldManager::move_speed;
    WorldManager::x_offset += ctrl.MoveLeft() * WorldManager::move_speed;

    if (last_frames_speed_x.size() > (size_t)(WorldManager::moving_inertia_frames))
    {
        last_frames_speed_x.erase(last_frames_speed_x.begin());
        last_frames_speed_y.erase(last_frames_speed_y.begin());
    }

    if (ctrl.IsMoving())
    {
        WorldManager::x_offset += (ctrl.GetMouse().x - old_ctrl.GetMouse().x);
        WorldManager::y_offset += (ctrl.GetMouse().y - old_ctrl.GetMouse().y);
        last_frames_speed_x.push_back(ctrl.GetMouse().x - old_ctrl.GetMouse().x);
        last_frames_speed_y.push_back(ctrl.GetMouse().y - old_ctrl.GetMouse().y);
    }
    else
    {
        WorldManager::x_offset += GetAverage(last_frames_speed_x);
        WorldManager::y_offset += GetAverage(last_frames_speed_y);
        last_frames_speed_x.push_back(0);
        last_frames_speed_y.push_back(0);
    }

    if (ctrl.IsPinching() && ((WorldManager::zoom + (ctrl.GetPinch() - old_ctrl.GetPinch())) > 10))
    {
        CorrectOffset(  ctrl.GetMouse(),
                        (ctrl.GetPinch() - old_ctrl.GetPinch()));
        WorldManager::zoom += (ctrl.GetPinch() - old_ctrl.GetPinch());
    }

    // ACTIONS
    HandleActionCtrl(old_ctrl.ActionUp(), ctrl.ActionUp(), WorldManager::actions["up"],WorldManager::level, WorldManager::objects);
    HandleActionCtrl(old_ctrl.ActionRight(), ctrl.ActionRight(), WorldManager::actions["right"], WorldManager::level, WorldManager::objects);
    HandleActionCtrl(old_ctrl.ActionDown(), ctrl.ActionDown(), WorldManager::actions["down"], WorldManager::level, WorldManager::objects);
    HandleActionCtrl(old_ctrl.ActionLeft(), ctrl.ActionLeft(), WorldManager::actions["left"], WorldManager::level, WorldManager::objects);
    
    HandleActionCtrl(old_ctrl.ActionEnter(), ctrl.ActionEnter(), WorldManager::actions["enter"], WorldManager::level, WorldManager::objects);
    //////////

    SDL_Point scr_center = {renderer->GetWidth() / 2, renderer->GetHeight() / 2};

    if (WorldManager::zoom <= 1)
    {
        CorrectOffset(  ctrl.tIsWheel() ? ctrl.GetMouse() : scr_center,
                        WorldManager::zoom - 1);
        WorldManager::zoom = 1;
    }
    else
    {
        CorrectOffset(  ctrl.tIsWheel() ? ctrl.GetMouse() : scr_center,
                        ctrl.ZoomOut() * WorldManager::zoom_speed * -1 * WorldManager::zoom);
        WorldManager::zoom -= ctrl.ZoomOut() * WorldManager::zoom_speed * WorldManager::zoom;
    }

    if (WorldManager::zoom >= 1000)
    {
        CorrectOffset(  ctrl.tIsWheel() ? ctrl.GetMouse() : scr_center,
                        1000 - WorldManager::zoom);
        WorldManager::zoom = 1000;
    }
    else
    {
        CorrectOffset(  ctrl.tIsWheel() ? ctrl.GetMouse() : scr_center,
                        ctrl.ZoomIn() * WorldManager::zoom_speed * WorldManager::zoom);
        WorldManager::zoom += ctrl.ZoomIn() * WorldManager::zoom_speed * WorldManager::zoom;
    }

    WorldManager::world->Step(1.0f / 60.0f, WorldManager::physics_quality * 3, WorldManager::physics_quality);

    // CYCLES
    auto cycles = WorldManager::level.GetCycles();
    for (size_t i = 0; i < WorldManager::cyclesDelays.size(); i++)
    {
        if (--(WorldManager::cyclesDelays[i]) <= 0)
        {
            auto cycle = cycles[i];

            WorldManager::cyclesDelays[i] = cycle.delay;

            for (size_t j = 0; j < cycle.objects.size(); j++)
            {
                WorldManager::AddObject(cycle.objects[j]);
            }
        }
    }    
    /////////

    
    // LATER IT WILL BE CONSIDERED DEPRECATED AND DESTROYED
    // No, it will be moved to the PauseMenu
    if (ctrl.ReloadLevel() && !old_ctrl.ReloadLevel()){
        WorldManager::LoadLevel(WorldManager::level, renderer);
        
//         Network::SetRepo("https://raw.githubusercontent.com/Hammerill/Sand-Box2D-levels/main/levels");
// #ifdef Vita
//         Network::DownloadFile("ux0:Data/Sand-Box2D/levels", "index.json");
//         Network::DownloadFile("ux0:Data/Sand-Box2D/levels", "default_level/default_level.json");
// #else
//         Network::DownloadFile("./levels", "index.json");
//         Network::DownloadFile("./levels", "default_level/default_level.json");
// #endif
    }
    ///////////////////////////////////////////////////////

    // POBJECTS REGISTRATION
    for (int i = WorldManager::order.size() - 1; i >= 0; i--)
    {
        WorldManager::objects.push_back(WorldManager::order[i]);

        auto pobj = WorldManager::objects[WorldManager::objects.size() - 1];
        
        if (typeid(*pobj) == typeid(PObjBox))
        {
            dynamic_cast<PObjBox*>(pobj)->SetTexture(WorldManager::LoadTexture(pobj->GetParam("texture_path").asString(), renderer->GetRenderer()));
        }

        pobj->Register(WorldManager::world, renderer->GetRenderer());
        
        WorldManager::order.pop_back();        
    }
    ////////////////////////

    // OUT OF BORDERS CHECK
    for (size_t i = 0; i < WorldManager::objects.size(); i++)
    {
        if (WorldManager::objects[i]->GetBody()->GetPosition().x > 100 ||
            WorldManager::objects[i]->GetBody()->GetPosition().y > 100 ||
            WorldManager::objects[i]->GetBody()->GetPosition().x < -100 ||
            WorldManager::objects[i]->GetBody()->GetPosition().y < -100 )
        {
            WorldManager::DeleteObject(i);
        }
    }
    ///////////////////////
}

int renderedItemsCount;
void WorldManager::Render(Renderer* renderer, Controls ctrl)
{
    SDL_SetRenderDrawBlendMode(renderer->GetRenderer(), SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer->GetRenderer(), 0x32, 0x32, 0x32, 0); //BG OPTION TO WORK ON
    SDL_RenderClear(renderer->GetRenderer());

    renderedItemsCount = 0;
    for (size_t i = 0; i < WorldManager::objects.size(); i++)   
    {
        if (WorldManager::objects[i]->Render(   renderer->GetRenderer(), 
                                                WorldManager::x_offset, 
                                                WorldManager::y_offset, 
                                                WorldManager::zoom,
                                                renderer->GetWidth(),
                                                renderer->GetHeight()))
        {
            renderedItemsCount++;
        }        
    }

    if (WorldManager::isDebug && Font::GetLoaded())
    {
        SDL_Point mouse = ctrl.GetMouse();

        std::vector<std::string> debugStrings;

        debugStrings.push_back("DEBUG");
        debugStrings.push_back("");
        debugStrings.push_back("Camera offset X = " + std::to_string(WorldManager::x_offset));
        debugStrings.push_back("Camera offset Y = " + std::to_string(WorldManager::y_offset));
        debugStrings.push_back("Zoom = " + std::to_string(WorldManager::zoom));
        debugStrings.push_back("Mouse X = " + std::to_string(mouse.x));
        debugStrings.push_back("Mouse Y = " + std::to_string(mouse.y));
        debugStrings.push_back("IsMoving? = " + std::to_string(ctrl.IsMoving()));
        debugStrings.push_back("Pinch = " + std::to_string(ctrl.GetPinch()));
        debugStrings.push_back("IsPinching? = " + std::to_string(ctrl.IsPinching()));
        debugStrings.push_back("Zoom In = " + std::to_string(ctrl.ZoomIn()));
        debugStrings.push_back("Zoom Out = " + std::to_string(ctrl.ZoomOut()));
        debugStrings.push_back("Objects count = " + std::to_string(WorldManager::world->GetBodyCount()));
        debugStrings.push_back("Objects rendered = " + std::to_string(renderedItemsCount));
        debugStrings.push_back("");
        debugStrings.push_back("ACTIONS");
        debugStrings.push_back("Up = " + std::to_string(ctrl.ActionUp()));
        debugStrings.push_back("Right = " + std::to_string(ctrl.ActionRight()));
        debugStrings.push_back("Down = " + std::to_string(ctrl.ActionDown()));
        debugStrings.push_back("Left = " + std::to_string(ctrl.ActionLeft()));
        debugStrings.push_back("Enter = " + std::to_string(ctrl.ActionEnter()));

        WorldManager::RenderDebugScreen(debugStrings, renderer);        
    }
}

void WorldManager::RenderDebugScreen(std::vector<std::string> debugStrings, Renderer* renderer)
{
    float debugScale = 2;

    std::vector<int> debugWidths;
    for (size_t i = 0; i < debugStrings.size(); i++)
        debugWidths.push_back((debugStrings[i].size() + 2) * Font::FontWidth * debugScale);

    int debug_w = *std::max_element(debugWidths.begin(), debugWidths.end());
    int debug_h = (debugStrings.size() + 2) * Font::FontWidth * debugScale;

    SDL_Rect debugBg {0, 0, debug_w, debug_h};

    SDL_SetRenderDrawBlendMode(renderer->GetRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer->GetRenderer(), 4, 4, 4, 0xA0);

    SDL_RenderFillRect(renderer->GetRenderer(), &debugBg);

    for (size_t i = 0; i < debugStrings.size(); i++)
    {
        Font::Render(renderer->GetRenderer(), debugStrings[i].c_str(), Font::FontWidth * debugScale, Font::FontWidth * debugScale * (i+1), debugScale);
    }
}

void WorldManager::CorrectOffset(SDL_Point mouse, float zoom_change)
{
    WorldManager::x_offset += ((WorldManager::x_offset - mouse.x) / WorldManager::zoom) * zoom_change;
    WorldManager::y_offset += ((WorldManager::y_offset - mouse.y) / WorldManager::zoom) * zoom_change;
}
