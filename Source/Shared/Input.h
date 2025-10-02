#pragma once

struct InputState
{
    bool currentKeys[256];
    bool previousKeys[256];

    bool IsKeyDown(int key) const 
    {
        return (key >= 0 && key < 256) ? currentKeys[key] : false;
    }

    bool IsKeyPressed(int key) const 
    {
        return (key >= 0 && key < 256) ? (currentKeys[key] && !previousKeys[key]) : false;
    }
}; 