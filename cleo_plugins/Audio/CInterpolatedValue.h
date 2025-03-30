#pragma once

class CInterpolatedValue
{
public:
    CInterpolatedValue(float value)
    {
        currValue = value;
        targetValue = value;
        step = 0.0f;
    }

    float value() const { return currValue; }

    void setValue(float target, float transitionTime)
    {
        targetValue = target;

        if (transitionTime <= 0.0f)
        {
            currValue = target;
            return;
        }

        step = (targetValue - currValue) / transitionTime;
    }

    void update(float elapsedTime)
    {
        if (currValue == targetValue)
        {
            return; // done
        }

        currValue += step * elapsedTime;

        // check progress
        auto remaining = targetValue - currValue;
        remaining *= (step > 0.0f) ? 1.0f : -1.0f;
        if (remaining <= 0.0f) // overshoot
        {
            currValue = targetValue;
        }
    }

    void finish()
    {
        currValue = targetValue;
    }

private:
    float currValue = 0.0f;
    float targetValue = 0.0f;
    float step = 0.0f;
};
