/**
 * The exception module defines all system-level exceptions and provides a
 * mechanism to alter system-level error handling.
 *
 * Copyright: Copyright Sean Kelly 2005 - 2009.
 * License:   <a href="http://www.boost.org/LICENSE_1_0.txt>Boost License 1.0</a>.
 * Authors:   Sean Kelly
 *
 *          Copyright Sean Kelly 2005 - 2009.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
module rt.util.string;

private import core.stdc.string;

char[] intToString( char[] buf, uint val )
{
    assert( buf.length > 9 );
    auto p = buf.ptr + buf.length;

    do
    {
        *--p = cast(char)(val % 10 + '0');
    } while( val /= 10 );

    return buf[p - buf.ptr .. $];
}


int dstrcmp( in char[] s1, in char[] s2 )
{
    auto len = s1.length;
    if( s2.length < len )
        len = s2.length;
    if( memcmp( s1.ptr, s2.ptr, len ) == 0 )
        return 0;
    return s1.length > s2.length ? 1 : -1;
}
