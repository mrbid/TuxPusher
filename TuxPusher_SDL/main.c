/*
    James William Fletcher (github.com/mrbid)
        July 2022 - January 2023

    https://SeaPusher.com & https://pusha.one

    A highly optimised and efficient coin pusher
    implementation.

    This was not made in a way that is easily extensible,
    the code can actually seem kind of confusing to
    initially understand.

    The only niggle is the inability to remove depth buffering
    when rendering the game over text due to the way I exported
    the mesh, disabling depth results in the text layers inverting
    causing the shadow to occlude the text face. It's not the
    end of the world, but could be fixed.

    Unlike the CoinPusher/SeaPusher release;
    https://github.com/mrbid/coinpusher this version does not
    allow custom framerates, just whatever your screen refresh
    rate is set to. I just wanted to be a little simple/regular
    in this release.

    Also in this one I step the collisions 6 times per frame
    rather than just once, which is 6x better quality, this
    prevents noticable overlap at higher input speeds which
    can now be set via the second argv.
*/

#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

#ifndef __x86_64__
    #define NOSSE
#endif

#include "esAux2.h"
#include "res.h"

#include "../assets/scene.h"
#include "../assets/coin.h"
#include "../assets/tux.h"
#include "../assets/evil.h"
#include "../assets/king.h"
#include "../assets/ninja.h"
#include "../assets/surf.h"
#include "../assets/trip.h"
#include "../assets/gameover.h"

#define uint GLushort // it's a short don't forget that
#define sint GLshort  // and this.
#define f32 GLfloat
#define forceinline __attribute__((always_inline)) inline

//*************************************
// globals
//*************************************
char appTitle[] = "TuxPusher";
SDL_Window* wnd;
SDL_GLContext glc;
SDL_Surface* s_icon = NULL;
SDL_Cursor* cross_cursor;
SDL_Cursor* beam_cursor;
uint cursor_state = 0;
uint winw = 1024, winh = 768;
f32 aspect;
f32 t = 0.f;
f32 dt = 0;   // delta time
f32 fc = 0;   // frame count
f32 lfct = 0; // last frame count time
f32 rww, ww, rwh, wh, ww2, wh2;
f32 uw, uh, uw2, uh2; // normalised pixel dpi
f32 touch_margin = 120.f;
f32 mx=0, my=0, md=0;
uint ortho = 0;

// render state id's
GLint projection_id;
GLint modelview_id;
GLint position_id;
GLint lightpos_id;
GLint solidcolor_id;
GLint color_id;
GLint opacity_id;
GLint normal_id;

// render state matrices
mat projection;
mat view;
mat model;
mat modelview;

// render state inputs
vec lightpos = {0.f, 10.f, 13.f};

// models
ESModel mdlGameover;
ESModel mdlScene;
ESModel mdlCoin;
ESModel mdlTux;
ESModel mdlEvil;
ESModel mdlKing;
ESModel mdlSurf;
ESModel mdlNinja;
ESModel mdlTrip;

// game vars
f32 gold_stack = 64.f;  // line 740+ defining these as float32 eliminates the need to cast in mTranslate()
f32 silver_stack = 64.f;// function due to the use of a float32 also in the for(f32 i;) loop.
uint active_coin = 0;
uint inmotion = 0;
double gameover = 0;
uint isnewcoin = 0;
f32 PUSH_SPEED;

typedef struct
{
    f32 x, y, r;
    signed char color;
} coin; // 4+4+4+1 = 13 bytes, 3 bytes padding to 16 byte cache line
#define MAX_COINS 130
coin coins[MAX_COINS] = {0};

uint trophies[6] = {0};

//*************************************
// game functions
//*************************************
void timestamp(char* ts)
{
    const time_t tt = time(0);
    strftime(ts, 16, "%H:%M:%S", localtime(&tt));
}

f32 f32Time()
{
    return ((f32)SDL_GetTicks())*0.001f;
}

void setActiveCoin(const uint color)
{
    for(int i=0; i < MAX_COINS; i++)
    {
        if(coins[i].color == -1)
        {
            coins[i].color = color;
            active_coin = i;
            return;
        }
    }
}

void takeStack()
{
    if(gameover == 0)
    {
        if(silver_stack != 0.f)
        {
            // play a silver coin
            isnewcoin = 1;
            setActiveCoin(0);
            inmotion = 1;
        }
        else if(gold_stack != 0.f)
        {
            // play a gold coin
            isnewcoin = 2;
            setActiveCoin(1);
            inmotion = 1;
        }

        if(inmotion == 1)
        {
            if(mx < touch_margin)
            {
                coins[active_coin].x = -1.90433f;
                coins[active_coin].y = -4.54055f;
            }
            else if(mx > ww-touch_margin)
            {
                coins[active_coin].x = 1.90433f;
                coins[active_coin].y = -4.54055f;
            }
            else
            {
                coins[active_coin].x = -1.90433f+(((mx-touch_margin)*rww)*3.80866f);
                coins[active_coin].y = -4.54055f;
            }
        }
    }
}

void injectFigure()
{
    if(inmotion != 0)
        return;
    
    int fc = -1;
    for(int i=0; i < 3; i++)
    {
        if(coins[i].color == -1)
        {
            active_coin = i;
            fc = i;
            coins[i].color = esRand(1, 6);
            break;
        }
    }

    if(fc != -1)
    {
        coins[active_coin].x = esRandFloat(-1.90433f, 1.90433f);
        coins[active_coin].y = -4.54055f;
        inmotion = 1;
    }
}

int insidePitch(const f32 x, const f32 y, const f32 r)
{
    // off bottom
    if(y < -4.03414f+r)
        return 0;
    
    // first left & right
    if(y < -2.22855f)
    {
        if(x < (-2.22482f - (0.77267f*(fabsf(y+4.03414f) * 0.553835588f))) + r)
            return 0;
        else if(x > (2.22482f + (0.77267f*(fabsf(y+4.03414f) * 0.553835588f))) - r)
            return 0;
    }
    else if(y < -0.292027f) // second left & right
    {
        if(x < (-2.99749f - (0.41114f*(fabsf(y+2.22855f) * 0.516389426f))) + r)
            return 0;
        else if(x > (2.99749f + (0.41114f*(fabsf(y+2.22855f) * 0.516389426f))) - r)
            return 0;
    }
    else if(y < 1.45439f) // third left & right
    {
        if(x < -3.40863f + r)
            return 0;
        else if(x > 3.40863f - r)
            return 0;
    }

    return 1;
}

int collision(int ci)
{
    for(int i=0; i < MAX_COINS; i++)
    {
        if(i == ci || coins[i].color == -1){continue;}
        const f32 xm = (coins[i].x - coins[ci].x);
        const f32 ym = (coins[i].y - coins[ci].y);
        const f32 radd = coins[i].r+coins[ci].r;
        if(xm*xm + ym*ym < radd*radd)
            return 1;
    }
    return 0;
}

uint stepCollisions()
{
    uint was_collision = 0;
    for(int i=0; i < MAX_COINS; i++)
    {
        if(coins[i].color == -1){continue;}
        for(int j=0; j < MAX_COINS; j++)
        {
            if(i == j || coins[j].color == -1 || j == active_coin){continue;}
            f32 xm = (coins[i].x - coins[j].x);
            xm += esRandFloat(-0.01f, 0.01f); // add some random offset to our unit vector, very subtle but works so well!
            const f32 ym = (coins[i].y - coins[j].y);
            f32 d = xm*xm + ym*ym;
            const f32 cr = coins[i].r+coins[j].r;
            if(d < cr*cr)
            {
                d = sqrtps(d);
                const f32 len = 1.f/d;
                const f32 uy = (ym * len);
                if(uy > 0.f){continue;} // best hack ever to massively simplify
                const f32 m = d-cr;
                coins[j].x += (xm * len) * m;
                coins[j].y += uy * m;

                // first left & right
                if(coins[j].y < -2.22855f)
                {
                    const f32 fl = (-2.22482f - (0.77267f*(fabsf(coins[j].y+4.03414f) * 0.553835588f))) + coins[j].r;
                    if(coins[j].x < fl)
                    {
                        coins[j].x = fl;
                    }
                    else
                    {
                        const f32 fr = ( 2.22482f + (0.77267f*(fabsf(coins[j].y+4.03414f) * 0.553835588f))) - coins[j].r;
                        if(coins[j].x > fr)
                            coins[j].x = fr;
                    }
                }
                else if(coins[j].y < -0.292027f) // second left & right
                {
                    const f32 fl = (-2.99749f - (0.41114f*(fabsf(coins[j].y+2.22855f) * 0.516389426f))) + coins[j].r;
                    if(coins[j].x < fl)
                    {
                        coins[j].x = fl;
                    }
                    else
                    {
                        const f32 fr = (2.99749f + (0.41114f*(fabsf(coins[j].y+2.22855f) * 0.516389426f))) - coins[j].r;
                        if(coins[j].x > fr)
                            coins[j].x = fr;
                    }
                }
                else if(coins[j].y < 1.45439f) // third left & right
                {
                    const f32 fl = -3.40863f + coins[j].r;
                    if(coins[j].x < fl)
                    {
                        coins[j].x = fl;
                    }
                    else
                    {
                        const f32 fr = 3.40863f - coins[j].r;
                        if(coins[j].x > fr)
                            coins[j].x = fr;
                    }
                }
                else if(coins[j].y < 2.58397f) // first house goal
                {
                    const f32 fl = (-3.40863f + (0.41113f*(fabsf(coins[j].y-1.45439f) * 0.885284796f)));
                    if(coins[j].x < fl)
                    {
                        coins[j].color = -1;
                    }
                    else
                    {
                        const f32 fr = (3.40863f - (0.41113f*(fabsf(coins[j].y-1.45439f) * 0.885284796f)));
                        if(coins[j].x > fr)
                            coins[j].color = -1;
                    }
                }
                else if(coins[j].y < 3.70642f) // second house goal
                {
                    const f32 fl = (-2.9975f + (1.34581f*(fabsf(coins[j].y-2.58397f) * 0.890908281f)));
                    if(coins[j].x < fl)
                    {
                        coins[j].color = -1;
                    }
                    else
                    {
                        const f32 fr = (2.9975f - (1.34581f*(fabsf(coins[j].y-2.58397f) * 0.890908281f)));
                        if(coins[j].x > fr)
                            coins[j].color = -1;
                    }
                }
                else if(coins[j].y < 4.10583f) // silver goal
                {
                    const f32 fl = (-1.65169f + (1.067374f*(fabsf(coins[j].y-3.70642f) * 2.503692947f)));
                    if(coins[j].x < fl)
                    {
                        if(j >= 0 && j <= 3)
                        {
                            if(trophies[coins[j].color-1] == 1) // already have? then reward coins!
                            {
                                gold_stack += 6.f;
                                silver_stack += 6.f;
                            }
                            else
                                trophies[coins[j].color-1] = 1;
                        }
                        else
                        {
                            if(coins[j].color == 0)
                                silver_stack += 1.f;
                            else if(coins[j].color == 1)
                                silver_stack += 2.f;
                        }

                        coins[j].color = -1;
                    }
                    else
                    {
                        const f32 fr = (1.65169f - (1.067374f*(fabsf(coins[j].y-3.70642f) * 2.503692947f)));
                        if(coins[j].x > fr)
                        {
                            if(j >= 0 && j <= 3)
                            {
                                if(trophies[coins[j].color-1] == 1) // already have? then reward coins!
                                {
                                    gold_stack += 6.f;
                                    silver_stack += 6.f;
                                }
                                else
                                    trophies[coins[j].color-1] = 1;
                            }
                            else
                            {
                                if(coins[j].color == 0)
                                    silver_stack += 1.f;
                                else if(coins[j].color == 1)
                                    silver_stack += 2.f;
                            }

                            coins[j].color = -1;
                        }
                    }
                }
                else if(coins[j].y >= 4.31457f) // gold goal
                {
                    if(coins[j].x >= -0.584316f && coins[j].x <= 0.584316f)
                    {
                        if(j >= 0 && j <= 3)
                        {
                            if(trophies[coins[j].color-1] == 1) // already have? then reward coins!
                            {
                                gold_stack += 6.f;
                                silver_stack += 6.f;
                            }
                            else
                                trophies[coins[j].color-1] = 1;
                        }
                        else
                        {
                            if(coins[j].color == 0)
                                gold_stack += 1.f;
                            else if(coins[j].color == 1)
                                gold_stack += 2.f;
                        }

                        coins[j].color = -1;
                    }
                    else
                    {
                        if(j >= 0 && j <= 3)
                        {
                            if(trophies[coins[j].color-1] == 1) // already have? then reward coins!
                            {
                                gold_stack += 6.f;
                                silver_stack += 6.f;
                            }
                            else
                                trophies[coins[j].color-1] = 1;
                        }
                        else
                            silver_stack += 1.f;

                        coins[j].color = -1;
                    }
                }
                
                was_collision++;
            }
        }
    }
    return was_collision;
}

void newGame()
{
    // seed randoms
    srand(time(0));

    // defaults
    gold_stack = 64.f;
    silver_stack = 64.f;
    active_coin = 0;
    inmotion = 0;
    gameover = 0;
    trophies[0] = 0;
    trophies[1] = 0;
    trophies[2] = 0;
    trophies[3] = 0;
    trophies[4] = 0;
    trophies[5] = 0;
    for(int i=0; i < MAX_COINS; i++)
    {
        coins[i].color = -1;
        coins[i].r = 0.3f;
    }

    // trophies
    for(int i=0; i < 3; i++)
    {
        coins[i].color = esRand(1, 6);
        coins[i].r = 0.24f;

        coins[i].x = esRandFloat(-3.40863f, 3.40863f);
        coins[i].y = esRandFloat(-4.03414f, 1.45439f-coins[i].r);
        while(insidePitch(coins[i].x, coins[i].y, coins[i].r) == 0 || collision(i) == 1)
        {
            coins[i].x = esRandFloat(-3.40863f, 3.40863f);
            coins[i].y = esRandFloat(-4.03414f, 1.45439f-coins[i].r);
        }
    }

    // coins
    const double lt = f32Time();
    for(int i=3; i < MAX_COINS; i++)
    {
        coins[i].x = esRandFloat(-3.40863f, 3.40863f);
        coins[i].y = esRandFloat(-4.03414f, 1.45439f-coins[i].r);
        uint tl = 0;
        while(insidePitch(coins[i].x, coins[i].y, coins[i].r) == 0 || collision(i) == 1)
        {
            coins[i].x = esRandFloat(-3.40863f, 3.40863f);
            coins[i].y = esRandFloat(-4.03414f, 1.45439f-coins[i].r);
            if(f32Time()-lt > 0.033){tl=1;break;} // 33ms timeout
        }
        if(tl==1){break;}
        coins[i].color = esRand(0, 4);
        if(coins[i].color > 1){coins[i].color = 0;}
    }

    // const int mc2 = MAX_COINS/2;
    // for(int i=3; i < mc2; i++)
    // {
    //     coins[i].x = esRandFloat(-3.40863f, 3.40863f);
    //     coins[i].y = esRandFloat(-4.03414f, 1.45439f-coins[i].r);
    //     while(insidePitch(coins[i].x, coins[i].y, coins[i].r) == 0 || collision(i) == 1)
    //     {
    //         coins[i].x = esRandFloat(-3.40863f, 3.40863f);
    //         coins[i].y = esRandFloat(-4.03414f, 1.45439f-coins[i].r);
    //     }
    //     coins[i].color = esRand(0, 4);
    //     if(coins[i].color > 1){coins[i].color = 0;}
    // }
}

//*************************************
// render functions
//*************************************
forceinline void modelBind1(const ESModel* mdl)
{
    glBindBuffer(GL_ARRAY_BUFFER, mdl->vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdl->nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdl->iid);
}

forceinline void modelBind3(const ESModel* mdl)
{
    glBindBuffer(GL_ARRAY_BUFFER, mdl->cid);
    glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(color_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdl->vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdl->nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdl->iid);
}

//*************************************
// emscripten/gl functions
//*************************************
void doPerspective()
{
    glViewport(0, 0, winw, winh);

    ww = winw;
    wh = winh;
    if(ortho == 1){touch_margin = ww*0.3076923192f;}
    else{touch_margin = ww*0.2058590651f;}
    rww = 1.f/(ww-touch_margin*2.f);
    rwh = 1.f/wh;
    ww2 = ww/2.f;
    wh2 = wh/2.f;
    uw = (double)aspect / ww;
    uh = 1.f / wh;
    uw2 = (double)aspect / ww2;
    uh2 = 1.f / wh2;

    mIdent(&projection);

    if(ortho == 1)
        mOrtho(&projection, -5.0f, 5.0f, -3.2f, 3.4f, 0.01f, 320.f);
    else
    {
        aspect = (f32)winw / (f32)winh;
        mPerspective(&projection, 30.0f, aspect, 0.01f, 320.f);
    }

}

//*************************************
// update & render
//*************************************
void main_loop()
{
//*************************************
// time delta for interpolation
//*************************************
    static f32 lt = 0;
    t = f32Time();
    dt = t-lt;
    lt = t;

//*************************************
// input handling
//*************************************
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_MOUSEMOTION:
            {
                mx = event.motion.x;
                my = event.motion.y;
            }
            break;

            case SDL_MOUSEBUTTONDOWN:
            {
                if(inmotion == 0 && event.button.button == SDL_BUTTON_LEFT)
                {
                    if(gameover > 0)
                    {
                        if(f32Time() > gameover+3.0)
                        {
                            newGame();
                            if(PUSH_SPEED < 32.f) // why not
                            {
                                PUSH_SPEED += 1.f;
                                char titlestr[256];
                                sprintf(titlestr, "TuxPusher [%.1f]", PUSH_SPEED);
                                SDL_SetWindowTitle(wnd, titlestr);
                            }
                        }
                        return;
                    }
                    takeStack();
                    md = 1;
                }

                if(event.button.button == SDL_BUTTON_RIGHT)
                {
                    static uint cs = 1;
                    cs = 1 - cs;
                    if(cs == 0)
                        SDL_ShowCursor(0);
                    else
                        SDL_ShowCursor(1);
                }
            }
            break;

            case SDL_MOUSEBUTTONUP:
            {
                if(event.button.button == SDL_BUTTON_LEFT)
                {
                    inmotion = 1;
                    md = 0;
                }
            }
            break;

            case SDL_KEYDOWN:
            {
                if(event.key.keysym.sym == SDLK_f)
                {
                    if(t-lfct > 2.0)
                    {
                        char strts[16];
                        timestamp(&strts[0]);
                        const double nfps = fc/(t-lfct);
                        printf("[%s] FPS: %g\n", strts, nfps);
                        lfct = t;
                        fc = 0;
                    }
                }
                else if(event.key.keysym.sym == SDLK_c)
                {
                    ortho = 1 - ortho;
                    doPerspective();
                }
            }
            break;

            case SDL_WINDOWEVENT:
            {
                if(event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    winw = event.window.data1;
                    winh = event.window.data2;
                    doPerspective();
                }
            }
            break;

            case SDL_QUIT:
            {
                SDL_GL_DeleteContext(glc);
                SDL_FreeSurface(s_icon);
                SDL_FreeCursor(cross_cursor);
                SDL_FreeCursor(beam_cursor);
                SDL_DestroyWindow(wnd);
                SDL_Quit();
                exit(0);
            }
            break;
        }
    }
    
//*************************************
// begin render
//*************************************
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//*************************************
// main render
//*************************************

    // cursor
    if(cursor_state == 0 && mx < touch_margin-1)
    {
        SDL_SetCursor(beam_cursor);
        cursor_state = 1;
    }
    else if(cursor_state == 0 && mx > ww-touch_margin+1)
    {
        SDL_SetCursor(beam_cursor);
        cursor_state = 1;
    }
    else if(cursor_state == 1 && mx > touch_margin && mx < ww-touch_margin)
    {
        SDL_SetCursor(cross_cursor);
        cursor_state = 0;
    }
    
    // camera
    mIdent(&view);
    mTranslate(&view, 0.f, 0.f, -13.f);
    if(ortho == 1)
        mRotY(&view, 50.f*DEG2RAD);
    else
        mRotY(&view, 62.f*DEG2RAD);

    // inject a new figure if time has come
    injectFigure();
    
    // prep scene for rendering
    shadeLambert3(&position_id, &projection_id, &modelview_id, &lightpos_id, &normal_id, &color_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    glUniform1f(opacity_id, 0.148f);

    // render scene
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &view.m[0][0]);
    modelBind3(&mdlScene);
    glDrawElements(GL_TRIANGLES, scene_numind, GL_UNSIGNED_SHORT, 0);

    // detect gameover
    if(gold_stack < 0.f){gold_stack = 0.f;}
    if(silver_stack < 0.f){silver_stack = 0.f;}
    if(gameover > 0 && (gold_stack != 0.f || silver_stack != 0.f))
    {
        gameover = 0;
    }
    else if(gold_stack == 0.f && silver_stack == 0.f)
    {
        if(gameover == 0)
            gameover = t+3.0;
    }

    // render game over
    if(gameover > 0 && t > gameover)
    {
        modelBind3(&mdlGameover);
        glDrawElements(GL_TRIANGLES, gameover_numind, GL_UNSIGNED_SHORT, 0);
    }

    // prep pieces for rendering
    shadeLambert1(&position_id, &projection_id, &modelview_id, &lightpos_id, &normal_id, &color_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    glUniform1f(opacity_id, 0.148f);

    // coin
    modelBind1(&mdlCoin);

    // targeting coin
    if(gold_stack > 0.f || silver_stack > 0.f)
    {
        if(coins[active_coin].color == 1)
            glUniform3f(color_id, 0.76471f, 0.63529f, 0.18824f);
        else
            glUniform3f(color_id, 0.68235f, 0.70196f, 0.72941f);
        if(md == 1)
        {
            if(mx < touch_margin)
            {
                coins[active_coin].x = -1.90433f;
                coins[active_coin].y = -4.54055f;
            }
            else if(mx > ww-touch_margin)
            {
                coins[active_coin].x = 1.90433f;
                coins[active_coin].y = -4.54055f;
            }
            else
            {
                coins[active_coin].x = -1.90433f+(((mx-touch_margin)*rww)*3.80866f);
                coins[active_coin].y = -4.54055f;
            }
        }
        else if(inmotion == 0)
        {
            if(silver_stack > 0.f)
                glUniform3f(color_id, 0.68235f, 0.70196f, 0.72941f);
            else
                glUniform3f(color_id, 0.76471f, 0.63529f, 0.18824f);

            mIdent(&model);

            if(mx < touch_margin)
                mTranslate(&model, -1.90433f, -4.54055f, 0);
            else if(mx > ww-touch_margin)
                mTranslate(&model, 1.90433f, -4.54055f, 0);
            else
                mTranslate(&model, -1.90433f+(((mx-touch_margin)*rww)*3.80866f), -4.54055f, 0);

            mMul(&modelview, &model, &view);
            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
            glDrawElements(GL_TRIANGLES, coin_numind, GL_UNSIGNED_SHORT, 0);
        }
    }

    // do motion
    if(inmotion == 1)
    {
        if(coins[active_coin].y < -3.73414f)
        {
            coins[active_coin].y += PUSH_SPEED * dt;
            for(int i=0; i < 6; i++) // six seems enough
                stepCollisions();
        }
        else
        {
            inmotion = 0;

            if(isnewcoin > 0)
            {
                if(isnewcoin == 1)
                    silver_stack -= 1.f;
                else
                    gold_stack -= 1.f;

                isnewcoin = 0;
            }
        }
    }

    // gold stack
    glUniform3f(color_id, 0.76471f, 0.63529f, 0.18824f);
    f32 gss = gold_stack;
    if(silver_stack == 0.f){gss -= 1.f;}
    if(gss < 0.f){gss = 0.f;}
    for(f32 i = 0.f; i < gss; i += 1.f)
    {
        mIdent(&model);
        if(ortho == 0)
            mTranslate(&model, -2.62939f, -4.54055f, 0.0406f*i);
        else
            mTranslate(&model, -4.62939f, -4.54055f, 0.0406f*i);
        mMul(&modelview, &model, &view);
        glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
        glDrawElements(GL_TRIANGLES, coin_numind, GL_UNSIGNED_SHORT, 0);
    }

    // silver stack
    glUniform3f(color_id, 0.68235f, 0.70196f, 0.72941f);
    f32 sss = silver_stack-1.f;
    if(sss < 0.f){sss = 0.f;}
    for(f32 i = 0.f; i < sss; i += 1.f)
    {
        mIdent(&model);
        if(ortho == 0)
            mTranslate(&model, 2.62939f, -4.54055f, 0.0406f*i);
        else
            mTranslate(&model, 4.62939f, -4.54055f, 0.0406f*i);
        mMul(&modelview, &model, &view);
        glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
        glDrawElements(GL_TRIANGLES, coin_numind, GL_UNSIGNED_SHORT, 0);
    }

    // pitch coins
    for(int i=3; i < MAX_COINS; i++)
    {
        if(coins[i].color == -1)
            continue;
        
        if(coins[i].color == 0)
            glUniform3f(color_id, 0.68235f, 0.70196f, 0.72941f);
        else
            glUniform3f(color_id, 0.76471f, 0.63529f, 0.18824f);

        mIdent(&model);
        mTranslate(&model, coins[i].x, coins[i].y, 0.f);
        mMul(&modelview, &model, &view);
        glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
        glDrawElements(GL_TRIANGLES, coin_numind, GL_UNSIGNED_SHORT, 0);
    }

    // tux is fancy
    shadeLambert3(&position_id, &projection_id, &modelview_id, &lightpos_id, &normal_id, &color_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    glUniform1f(opacity_id, 0.148f);

    // trophies
    for(int i=0; i < 3; i++)
    {
        if(coins[i].color == 1)
        {
            mIdent(&model);
            mTranslate(&model, coins[i].x, coins[i].y, 0.f);
            mMul(&modelview, &model, &view);
            
            glUniform1f(opacity_id, 0.148f);
            modelBind3(&mdlTux);
            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
            glDrawElements(GL_TRIANGLES, tux_numind, GL_UNSIGNED_SHORT, 0);
        }
        else if(coins[i].color == 2)
        {
            mIdent(&model);
            mTranslate(&model, coins[i].x, coins[i].y, 0.f);
            mMul(&modelview, &model, &view);
            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);

            glUniform1f(opacity_id, 0.148f);
            modelBind3(&mdlTux);
            glDrawElements(GL_TRIANGLES, tux_numind, GL_UNSIGNED_SHORT, 0);
            
            glUniform1f(opacity_id, 0.5f);
            modelBind3(&mdlEvil);
            glDrawElements(GL_TRIANGLES, evil_numind, GL_UNSIGNED_BYTE, 0);
        }
        else if(coins[i].color == 3)
        {
            mIdent(&model);
            mTranslate(&model, coins[i].x, coins[i].y, 0.f);
            mMul(&modelview, &model, &view);
            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);

            glUniform1f(opacity_id, 0.148f);
            modelBind3(&mdlTux);
            glDrawElements(GL_TRIANGLES, tux_numind, GL_UNSIGNED_SHORT, 0);

            glUniform1f(opacity_id, 0.6f);
            modelBind3(&mdlKing);
            glDrawElements(GL_TRIANGLES, king_numind, GL_UNSIGNED_BYTE, 0);
        }
        else if(coins[i].color == 4)
        {
            mIdent(&model);
            mTranslate(&model, coins[i].x, coins[i].y, 0.f);
            mMul(&modelview, &model, &view);
            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);

            glUniform1f(opacity_id, 0.148f);
            modelBind3(&mdlTux);
            glDrawElements(GL_TRIANGLES, tux_numind, GL_UNSIGNED_SHORT, 0);

            modelBind3(&mdlNinja);
            glDrawElements(GL_TRIANGLES, ninja_numind, GL_UNSIGNED_BYTE, 0);
        }
        else if(coins[i].color == 5)
        {
            mIdent(&model);
            mTranslate(&model, coins[i].x, coins[i].y, 0.f);
            mMul(&modelview, &model, &view);
            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);

            glUniform1f(opacity_id, 0.148f);
            modelBind3(&mdlTux);
            glDrawElements(GL_TRIANGLES, tux_numind, GL_UNSIGNED_SHORT, 0);
            
            glUniform1f(opacity_id, 0.4f);
            modelBind3(&mdlSurf);
            glDrawElements(GL_TRIANGLES, surf_numind, GL_UNSIGNED_BYTE, 0);
        }
        else if(coins[i].color == 6)
        {
            mIdent(&model);
            mTranslate(&model, coins[i].x, coins[i].y, 0.f);
            mMul(&modelview, &model, &view);
            glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);

            glUniform1f(opacity_id, 0.148f);
            modelBind3(&mdlTux);
            glDrawElements(GL_TRIANGLES, tux_numind, GL_UNSIGNED_SHORT, 0);

            glUniform1f(opacity_id, 0.5f);
            modelBind3(&mdlTrip);
            glDrawElements(GL_TRIANGLES, trip_numind, GL_UNSIGNED_SHORT, 0);
        }
    }

    //

    if(trophies[0] == 1)
    {
        mIdent(&model);
        mTranslate(&model, 3.92732f, 1.0346f, 0.f);
        mRotZ(&model, t*0.3f);
        mMul(&modelview, &model, &view);
        glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);

        glUniform1f(opacity_id, 0.148f);
        modelBind3(&mdlTux);
        glDrawElements(GL_TRIANGLES, tux_numind, GL_UNSIGNED_SHORT, 0);
    }

    if(trophies[1] == 1)
    {
        mIdent(&model);
        mTranslate(&model, 3.65552f, -1.30202f, 0.f);
        mRotZ(&model, t*0.3f);
        mMul(&modelview, &model, &view);
        glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
        
        glUniform1f(opacity_id, 0.148f);
        modelBind3(&mdlTux);
        glDrawElements(GL_TRIANGLES, tux_numind, GL_UNSIGNED_SHORT, 0);

        glUniform1f(opacity_id, 0.5f);
        modelBind3(&mdlEvil);
        glDrawElements(GL_TRIANGLES, evil_numind, GL_UNSIGNED_BYTE, 0);
    }

    if(trophies[2] == 1)
    {
        mIdent(&model);
        mTranslate(&model, 3.01911f, -3.23534f, 0.f);
        mRotZ(&model, t*0.3f);
        mMul(&modelview, &model, &view);
        glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);

        glUniform1f(opacity_id, 0.148f);
        modelBind3(&mdlTux);
        glDrawElements(GL_TRIANGLES, tux_numind, GL_UNSIGNED_SHORT, 0);

        glUniform1f(opacity_id, 0.6f);
        modelBind3(&mdlKing);
        glDrawElements(GL_TRIANGLES, king_numind, GL_UNSIGNED_BYTE, 0);
    }

    if(trophies[3] == 1)
    {
        mIdent(&model);
        mTranslate(&model, -3.92732f, 1.0346f, 0.f);
        mRotZ(&model, t*0.3f);
        mMul(&modelview, &model, &view);
        glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);

        glUniform1f(opacity_id, 0.148f);
        modelBind3(&mdlTux);
        glDrawElements(GL_TRIANGLES, tux_numind, GL_UNSIGNED_SHORT, 0);

        modelBind3(&mdlNinja);
        glDrawElements(GL_TRIANGLES, ninja_numind, GL_UNSIGNED_BYTE, 0);
    }

    if(trophies[4] == 1)
    {
        mIdent(&model);
        mTranslate(&model, -3.65552f, -1.30202f, 0.f);
        mRotZ(&model, t*0.3f);
        mMul(&modelview, &model, &view);
        glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);

        glUniform1f(opacity_id, 0.148f);
        modelBind3(&mdlTux);
        glDrawElements(GL_TRIANGLES, tux_numind, GL_UNSIGNED_SHORT, 0);

        glUniform1f(opacity_id, 0.4f);
        modelBind3(&mdlSurf);
        glDrawElements(GL_TRIANGLES, surf_numind, GL_UNSIGNED_BYTE, 0);
    }

    if(trophies[5] == 1)
    {
        mIdent(&model);
        mTranslate(&model, -3.01911f, -3.23534f, 0.f);
        mRotZ(&model, t*0.3f);
        mMul(&modelview, &model, &view);
        glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);

        glUniform1f(opacity_id, 0.148f);
        modelBind3(&mdlTux);
        glDrawElements(GL_TRIANGLES, tux_numind, GL_UNSIGNED_SHORT, 0);

        glUniform1f(opacity_id, 0.5f);
        modelBind3(&mdlTrip);
        glDrawElements(GL_TRIANGLES, trip_numind, GL_UNSIGNED_SHORT, 0);
    }

//*************************************
// swap buffers / display render
//*************************************
    SDL_GL_SwapWindow(wnd);
}

//*************************************
// Process Entry Point
//*************************************
SDL_Surface* surfaceFromData(const Uint32* data, Uint32 w, Uint32 h)
{
    SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    Uint32 i = 0;
    for(Uint32 y = 0; y < h; ++y)
    {
        for(Uint32 x = 0; x < w; ++x)
        {
            const Uint8* pixel = (Uint8*)s->pixels + (y * s->pitch) + (x * s->format->BytesPerPixel);
            *((Uint32*)pixel) = data[i++];
        }
    }
    return s;
}
int main(int argc, char** argv)
{
    // allow custom msaa level
    int msaa = 16;
    if(argc >= 2){msaa = atoi(argv[1]);}

    // help
    printf("----\n");
    printf("TuxPusher\n");
    printf("----\n");
    printf("James William Fletcher (github.com/mrbid)\n");
    printf("----\n");
    printf("Argv(2): msaa, speed\n");
    printf("e.g; ./uc 16\n");
    printf("----\n");
    printf("Left Click = Release coin\n");
    printf("Right Click = Show/hide cursor\n");
    printf("C = Orthographic/Perspective\n");
    printf("F = FPS to console\n");
    printf("----\n");
    printf("Tux 3D model by Andy Cuccaro:\n");
    printf("https://sketchfab.com/3d-models/tux-157de95fa4014050a969a8361a83d366\n");
    printf("----\n");
    printf("Rules:\n");
    printf("Getting a gold coin in a silver slot rewards you 2x silver coins.\n");
    printf("Getting a gold coin in a gold slot rewards you 2x gold coins.\n");
    printf("Getting a tux in a slot when you already have the tux gives you 6x gold coins and 6x silver coins.\n");
    printf("----\n");

    // init sdl
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS) < 0) //SDL_INIT_AUDIO
    {
        fprintf(stderr, "ERROR: SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaa);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    wnd = SDL_CreateWindow(appTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winw, winh, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    SDL_GL_SetSwapInterval(1);
    glc = SDL_GL_CreateContext(wnd);

    // set cursors
    cross_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
    beam_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
    SDL_SetCursor(cross_cursor);

    // set icon
    s_icon = surfaceFromData((Uint32*)&icon_image.pixel_data[0], 16, 16);
    SDL_SetWindowIcon(wnd, s_icon);

    // set game push speed
    PUSH_SPEED = 1.6f;
    if(argc >= 3)
    {
        PUSH_SPEED = atof(argv[2]);
        if(PUSH_SPEED > 32.f){PUSH_SPEED = 32.f;}
        char titlestr[256];
        sprintf(titlestr, "TuxPusher [%.1f]", PUSH_SPEED);
        SDL_SetWindowTitle(wnd, titlestr);
    }

    // debug
#ifdef GL_DEBUG
    esDebug(1);
#endif

//*************************************
// bind vertex and index buffers
//*************************************

    // ***** BIND SCENE *****
    esBind(GL_ARRAY_BUFFER, &mdlScene.cid, scene_colors, sizeof(scene_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlScene.vid, scene_vertices, sizeof(scene_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlScene.nid, scene_normals, sizeof(scene_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlScene.iid, scene_indices, sizeof(scene_indices), GL_STATIC_DRAW);

    // ***** BIND GAMEOVER *****
    esBind(GL_ARRAY_BUFFER, &mdlGameover.cid, gameover_colors, sizeof(gameover_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlGameover.vid, gameover_vertices, sizeof(gameover_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlGameover.nid, gameover_normals, sizeof(gameover_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlGameover.iid, gameover_indices, sizeof(gameover_indices), GL_STATIC_DRAW);

    // ***** BIND COIN *****
    esBind(GL_ARRAY_BUFFER, &mdlCoin.vid, coin_vertices, sizeof(coin_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlCoin.nid, coin_normals, sizeof(coin_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlCoin.iid, coin_indices, sizeof(coin_indices), GL_STATIC_DRAW);

    // ***** BIND TUX *****
    esBind(GL_ARRAY_BUFFER, &mdlTux.cid, tux_colors, sizeof(tux_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlTux.vid, tux_vertices, sizeof(tux_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlTux.nid, tux_normals, sizeof(tux_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlTux.iid, tux_indices, sizeof(tux_indices), GL_STATIC_DRAW);

    // ***** BIND TUX - EVIL *****
    esBind(GL_ARRAY_BUFFER, &mdlEvil.cid, evil_colors, sizeof(evil_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlEvil.vid, evil_vertices, sizeof(evil_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlEvil.nid, evil_normals, sizeof(evil_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlEvil.iid, evil_indices, sizeof(evil_indices), GL_STATIC_DRAW);

    // ***** BIND TUX - KING *****
    esBind(GL_ARRAY_BUFFER, &mdlKing.cid, king_colors, sizeof(king_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlKing.vid, king_vertices, sizeof(king_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlKing.nid, king_normals, sizeof(king_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlKing.iid, king_indices, sizeof(king_indices), GL_STATIC_DRAW);

    // ***** BIND TUX - NINJA *****
    esBind(GL_ARRAY_BUFFER, &mdlNinja.cid, ninja_colors, sizeof(ninja_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlNinja.vid, ninja_vertices, sizeof(ninja_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlNinja.nid, ninja_normals, sizeof(ninja_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlNinja.iid, ninja_indices, sizeof(ninja_indices), GL_STATIC_DRAW);

    // ***** BIND TUX - SURF *****
    esBind(GL_ARRAY_BUFFER, &mdlSurf.cid, surf_colors, sizeof(surf_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlSurf.vid, surf_vertices, sizeof(surf_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlSurf.nid, surf_normals, sizeof(surf_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlSurf.iid, surf_indices, sizeof(surf_indices), GL_STATIC_DRAW);

    // ***** BIND TUX - TRIP *****
    esBind(GL_ARRAY_BUFFER, &mdlTrip.cid, trip_colors, sizeof(trip_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlTrip.vid, trip_vertices, sizeof(trip_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlTrip.nid, trip_normals, sizeof(trip_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlTrip.iid, trip_indices, sizeof(trip_indices), GL_STATIC_DRAW);

//*************************************
// compile & link shader program
//*************************************

    makeLambert1();
    makeLambert3();

//*************************************
// configure render options
//*************************************

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.52941f, 0.80784f, 0.92157f, 0.0f);

//*************************************
// execute update / render loop
//*************************************

    // new game
    doPerspective();
    newGame();
    
    // init
    t = f32Time();
    lfct = t;
    
    // event loop
    while(1){main_loop();fc++;}
    return 0;
}

