#ifndef VRect_H
#define VRect_H
#include "VDimension.h"

namespace NervGear
{

template <class T>
    class VRect
    {
    public:

        //! Default constructor creating empty VRectangle at (0,0)
        VRect() : UpperLeftCorner(0,0), LowerRightCorner(0,0) {}

        //! Constructor with two corners
        VRect(T x, T y, T x2, T y2)
            : UpperLeftCorner(x,y), LowerRightCorner(x2,y2) {}

        //! Constructor with two corners
        VRect(const V2Vect<T>& upperLeft, const V2Vect<T>& lowerRight)
            : UpperLeftCorner(upperLeft), LowerRightCorner(lowerRight) {}

        //! Constructor with upper left corner and dimension
        template <class U>
        VRect(const V2Vect<T>& pos, const VDimension<U>& size)
            : UpperLeftCorner(pos), LowerRightCorner(pos.x + size.Width, pos.y + size.Height) {}

        //! move right by given numbers
        VRect<T> operator+(const V2Vect<T>& pos) const
        {
            VRect<T> ret(*this);
            return ret+=pos;
        }

        //! move right by given numbers
        VRect<T>& operator+=(const V2Vect<T>& pos)
        {
            UpperLeftCorner += pos;
            LowerRightCorner += pos;
            return *this;
        }

        //! move left by given numbers
        VRect<T> operator-(const V2Vect<T>& pos) const
        {
            VRect<T> ret(*this);
            return ret-=pos;
        }

        //! move left by given numbers
        VRect<T>& operator-=(const V2Vect<T>& pos)
        {
            UpperLeftCorner -= pos;
            LowerRightCorner -= pos;
            return *this;
        }

        //! equality operator
        bool operator==(const VRect<T>& other) const
        {
            return (UpperLeftCorner == other.UpperLeftCorner &&
                LowerRightCorner == other.LowerRightCorner);
        }

        //! inequality operator
        bool operator!=(const VRect<T>& other) const
        {
            return (UpperLeftCorner != other.UpperLeftCorner ||
                LowerRightCorner != other.LowerRightCorner);
        }

        //! compares size of VRectangles
        bool operator<(const VRect<T>& other) const
        {
            return getArea() < other.getArea();
        }

        //! Returns size of VRectangle
        T getArea() const
        {
            return getWidth() * getHeight();
        }

        //! Returns if a 2d point is within this VRectangle.
        /** \param pos Position to test if it lies within this VRectangle.
        \return True if the position is within the VRectangle, false if not. */
        bool isPointInside(const V2Vect<T>& pos) const
        {
            return (UpperLeftCorner.x <= pos.x &&
                UpperLeftCorner.y <= pos.y &&
                LowerRightCorner.x >= pos.x &&
                LowerRightCorner.y >= pos.y);
        }

        //! Check if the VRectangle collides with another VRectangle.
        /** \param other VRectangle to test collision with
        \return True if the VRectangles collide. */
        bool isVRectCollided(const VRect<T>& other) const
        {
            return (LowerRightCorner.y > other.UpperLeftCorner.y &&
                UpperLeftCorner.y < other.LowerRightCorner.y &&
                LowerRightCorner.x > other.UpperLeftCorner.x &&
                UpperLeftCorner.x < other.LowerRightCorner.x);
        }

        //! Clips this VRectangle with another one.
        /** \param other VRectangle to clip with */
        void clipAgainst(const VRect<T>& other)
        {
            if (other.LowerRightCorner.x < LowerRightCorner.x)
                LowerRightCorner.x = other.LowerRightCorner.x;
            if (other.LowerRightCorner.y < LowerRightCorner.y)
                LowerRightCorner.y = other.LowerRightCorner.y;

            if (other.UpperLeftCorner.x > UpperLeftCorner.x)
                UpperLeftCorner.x = other.UpperLeftCorner.x;
            if (other.UpperLeftCorner.y > UpperLeftCorner.y)
                UpperLeftCorner.y = other.UpperLeftCorner.y;

            // corVRect possible invalid VRect resulting from clipping
            if (UpperLeftCorner.y > LowerRightCorner.y)
                UpperLeftCorner.y = LowerRightCorner.y;
            if (UpperLeftCorner.x > LowerRightCorner.x)
                UpperLeftCorner.x = LowerRightCorner.x;
        }

        //! Moves this VRectangle to fit inside another one.
        /** \return True on success, false if not possible */
        bool constrainTo(const VRect<T>& other)
        {
            if (other.getWidth() < getWidth() || other.getHeight() < getHeight())
                return false;

            T diff = other.LowerRightCorner.x - LowerRightCorner.x;
            if (diff < 0)
            {
                LowerRightCorner.x += diff;
                UpperLeftCorner.x  += diff;
            }

            diff = other.LowerRightCorner.y - LowerRightCorner.y;
            if (diff < 0)
            {
                LowerRightCorner.y += diff;
                UpperLeftCorner.y  += diff;
            }

            diff = UpperLeftCorner.x - other.UpperLeftCorner.x;
            if (diff < 0)
            {
                UpperLeftCorner.x  -= diff;
                LowerRightCorner.x -= diff;
            }

            diff = UpperLeftCorner.y - other.UpperLeftCorner.y;
            if (diff < 0)
            {
                UpperLeftCorner.y  -= diff;
                LowerRightCorner.y -= diff;
            }

            return true;
        }

        //! Get width of VRectangle.
        T getWidth() const
        {
            return LowerRightCorner.x - UpperLeftCorner.x;
        }

        //! Get height of VRectangle.
        T getHeight() const
        {
            return LowerRightCorner.y - UpperLeftCorner.y;
        }

        //! If the lower right corner of the VRect is smaller then the upper left, the points are swapped.
        void repair()
        {
            if (LowerRightCorner.x < UpperLeftCorner.x)
            {
                T t = LowerRightCorner.x;
                LowerRightCorner.x = UpperLeftCorner.x;
                UpperLeftCorner.x = t;
            }

            if (LowerRightCorner.y < UpperLeftCorner.y)
            {
                T t = LowerRightCorner.y;
                LowerRightCorner.y = UpperLeftCorner.y;
                UpperLeftCorner.y = t;
            }
        }

        //! Returns if the VRect is valid to draw.
        /** It would be invalid if the UpperLeftCorner is lower or more
        right than the LowerRightCorner. */
        bool isValid() const
        {
            return ((LowerRightCorner.x >= UpperLeftCorner.x) &&
                (LowerRightCorner.y >= UpperLeftCorner.y));
        }

        //! Get the center of the VRectangle
        V2Vect<T> getCenter() const
        {
            return V2Vect<T>(
                    (UpperLeftCorner.x + LowerRightCorner.x) / 2,
                    (UpperLeftCorner.y + LowerRightCorner.y) / 2);
        }

        //! Get the dimensions of the VRectangle
        VDimension<T> getSize() const
        {
            return VDimension<T>(getWidth(), getHeight());
        }


        //! Adds a point to the VRectangle
        /** Causes the VRectangle to grow bigger if point is outside of
        the box
        \param p Point to add to the box. */
        void addInternalPoint(const V2Vect<T>& p)
        {
            addInternalPoint(p.x, p.y);
        }

        //! Adds a point to the bounding VRectangle
        /** Causes the VRectangle to grow bigger if point is outside of
        the box
        \param x X-Coordinate of the point to add to this box.
        \param y Y-Coordinate of the point to add to this box. */
        void addInternalPoint(T x, T y)
        {
            if (x>LowerRightCorner.x)
                LowerRightCorner.x = x;
            if (y>LowerRightCorner.y)
                LowerRightCorner.y = y;

            if (x<UpperLeftCorner.x)
                UpperLeftCorner.x = x;
            if (y<UpperLeftCorner.y)
                UpperLeftCorner.y = y;
        }

        //! Upper left corner
        V2Vect<T> UpperLeftCorner;
        //! Lower right corner
        V2Vect<T> LowerRightCorner;
    };

    //! VRectangle with float values
    typedef VRect<f32> VRectf;
    //! VRectangle with int values
    typedef VRect<s32> VRecti;


}


#endif // VVRect_H

