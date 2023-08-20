// copy a cairo win32 surface (with dib) to the clipboard.
// http://stevehanov.ca/blog/?id=35
//
bool
GraphicsClipboardSurface::copyToClipboard(HWND hwnd)
{
    cairo_surface_t* imageSurface = cairo_win32_surface_get_image( _surface );
    if ( imageSurface == NULL ) {
        assert(false);
        return false;
    }

    unsigned char* bits = cairo_image_surface_get_data( imageSurface );

    if ( bits == NULL ) {
        assert( false );
        return false;
    }

    assert( cairo_image_surface_get_format( imageSurface ) == CAIRO_FORMAT_RGB24 );

    BITMAPINFOHEADER bmi;
    unsigned biSizeImage;
    memset( &bmi, 0, sizeof(bmi) );
    bmi.biSize = sizeof(bmi);
    bmi.biWidth = cairo_image_surface_get_width( imageSurface );
    bmi.biHeight= cairo_image_surface_get_height( imageSurface ); 

    unsigned rowPad = ( 4 - ( ( bmi.biWidth * 3 ) & 3 ) ) & 3;

    bmi.biPlanes = 1;
    bmi.biBitCount = 24; // 24 or 32. If 32, high byte is not used.
    bmi.biCompression = BI_RGB;
    biSizeImage = bmi.biWidth * bmi.biHeight * ( bmi.biBitCount / 8 ) + bmi.biHeight * rowPad;
    bmi.biXPelsPerMeter = (LONG)((double)96 * 100 / 2.54 + 0.5) ; // dpix
    bmi.biYPelsPerMeter = (LONG)((double)96 * 100 / 2.54 + 0.5); // dpiy
    bmi.biClrUsed = 0;
    bmi.biClrImportant = 0;

    HGLOBAL hMem = NULL;
    unsigned char* ptr = 0;
    unsigned size;
    bool success = false;

    // OpenClipboard
    if ( !OpenClipboard(hwnd) ) {
        return false;
    }

    // call EmptyClipboard
    if ( !EmptyClipboard() ) {
        goto error;
    }

    // calculate size of the data.
    size = bmi.biSize + biSizeImage;

    // Allocate the data using GlobalAlloc with GMEM_MOVEABLE flag.
    hMem = GlobalAlloc( GMEM_MOVEABLE, size );

    if ( hMem == NULL ) {
        goto error;
    }

    ptr = (unsigned char*)GlobalLock( hMem );
    if ( ptr == 0 ) {
        goto error;
    }

    // copy data to clipboard
    memcpy( ptr, &bmi, bmi.biSize );

    // copy each row of the bitmap in reverse order, adding padding after each
    // row.
    unsigned char* src = bits + bmi.biWidth * (bmi.biHeight-1) * 4;
    unsigned char* dest = ptr + bmi.biSize;
    for ( int i = 0; i < bmi.biHeight; i++ ) {
        for ( int j = 0; j < bmi.biWidth; j++ ) {
            *dest++ = *src++;
            *dest++ = *src++;
            *dest++ = *src++;
            src++;
        }

        dest += rowPad;
        src -= bmi.biWidth * 4 * 2;
    }

    GlobalUnlock( hMem );

    // Call SetClipboardData
    if ( !SetClipboardData( CF_DIB, hMem ) ) {
        goto error;
    }

    hMem = NULL; 

    success = true;
error:
    if ( hMem ) {
        GlobalFree( hMem );
    }
    CloseClipboard();
    
    return success;


