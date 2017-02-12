#include<xcb/xcb.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>

#include"gui.h"
#include"data.h"
#include"argpass.h"
#include"errors.h"
#include"keys.h"

int windowHeight = 1;
int windowWidth = 0;
int windowX = 0;
int windowY = 0;

//Magic number, should be changed when changing font rendering method.
int lineHeight = 19;

int numberOfLinesToPrint;
char** linesToPrint = NULL;

xcb_connection_t* connection = NULL;
xcb_screen_t* screen = NULL;
xcb_drawable_t window = 0;

xcb_gc_t fontGC;
xcb_gc_t fillGC;

static void testCookie(xcb_void_cookie_t cookie, xcb_connection_t *connection, char *errMessage ) {
    xcb_generic_error_t *error = xcb_request_check (connection, cookie);
    if (error) {
        xcb_disconnect (connection);
        guiError(errMessage);
    }
}

void grabKeyboard(int iters) {
    int i = 0;
    while(1) {
        if ( xcb_connection_has_error ( connection ) ) {
            guiError("Error in connection while grabbing keyboard");
        }
        xcb_grab_keyboard_cookie_t cc = xcb_grab_keyboard(connection, 1, window, XCB_CURRENT_TIME, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
        xcb_grab_keyboard_reply_t *r = xcb_grab_keyboard_reply(connection, cc, NULL);
        if ( r ) {
            if ( r->status == XCB_GRAB_STATUS_SUCCESS ) {
                free ( r );
                return;
            }
            free ( r );
        }
        if ( (++i) > iters ) {
            break;
        }
        usleep ( 1000 );
    }
    guiError("failed grabbing a keyboard.");
}

void releaseKeyboard() {
    xcb_ungrab_keyboard ( connection, XCB_CURRENT_TIME );
}

void fontGCInit() {
    //get font
    xcb_font_t font = xcb_generate_id (connection);
    xcb_void_cookie_t fontCookie = xcb_open_font_checked(connection,font,strlen(arguments.font),arguments.font);
    testCookie(fontCookie,connection,"can't open font");

    //get graphics content
    fontGC = xcb_generate_id (connection);
    uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT;
    uint32_t value_list[3] = { arguments.fgColor, arguments.bgColor, font };
    xcb_create_gc(connection, fontGC, window, mask, value_list);

    //close font
    fontCookie = xcb_close_font_checked(connection, font);
    testCookie(fontCookie, connection, "can't close font");
}

void drawText(int16_t  x1, int16_t y1, const char *label ) {
    fontGCInit(arguments.font);
    xcb_void_cookie_t textCookie = xcb_image_text_8_checked(connection,strlen(label),window,fontGC,x1,y1,label);
    testCookie(textCookie,connection,"can't paste text");
    xcb_void_cookie_t gcCookie = xcb_free_gc(connection,fontGC);
    testCookie(gcCookie,connection,"can't free gc");
}

uint32_t calcXPos(){
    switch (arguments.winXPos) {
        case (XLeft)  : return 0 + arguments.winYOffset; break;
        case (XMid)   : return screen->width_in_pixels / 2 - windowWidth / 2 + arguments.winYOffset; break;
        case (XRight) : return screen->width_in_pixels - windowWidth + arguments.winYOffset; break;
        default: return 0;
    }
}

uint32_t calcYPos(){    
    switch (arguments.winYPos) {
        case (YTop) : return 0 + arguments.winXOffset; break;
        case (YMid) : return screen->height_in_pixels / 2 + arguments.winXOffset; break;
        case (YBot) : return screen->height_in_pixels + arguments.winXOffset; break;
        default: return 0;
    }
}

uint32_t calcHeight(){
    return (numberOfLinesToPrint * 20) + arguments.topIndent + arguments.botIndent;
}

void updateWindowGeometry() {
    //Tell x what we want
    uint32_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_HEIGHT;
    uint32_t values[3] = {calcXPos(), calcYPos(), calcHeight()};
    xcb_configure_window(connection, window, mask, values);
    //Unsure if i have to wait now before seeing what we got

    //Hear what we got
    xcb_get_geometry_cookie_t cookie = xcb_get_geometry(connection, window);
    xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(connection, cookie, NULL);
    windowX = reply->x;
    windowY = reply->y;
    windowHeight = reply->height;
}

void updateData() {
    if (linesToPrint == NULL) {
        linesToPrint = allocForDirToStrings();
    }
    dirToStrings(linesToPrint,&numberOfLinesToPrint);
}

void setWindowFlags() {
    xcb_intern_atom_cookie_t cookie1 = xcb_intern_atom(connection, 0, strlen("_NET_WM_WINDOW_TYPE"),"_NET_WM_WINDOW_TYPE");
    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(connection, 0, strlen("_NET_WM_WINDOW_TYPE_DOCK"), "_NET_WM_WINDOW_TYPE_DOCK");
    xcb_intern_atom_reply_t* reply1 = xcb_intern_atom_reply(connection, cookie1, 0);
    xcb_intern_atom_reply_t* reply2 = xcb_intern_atom_reply(connection, cookie2, 0);
    xcb_void_cookie_t cc1 = xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, reply1->atom, XCB_ATOM_ATOM, 32, 1, &(reply2->atom));
    xcb_void_cookie_t cc2 = xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_CLASS, XCB_ATOM_STRING, 8, strlen("blezz\0Blezz\0"), "blezz\0Blezz\0");
    xcb_void_cookie_t cc3 = xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen("Blezz"), "Blezz");
    testCookie(cc1, connection, "failed setting _NET_WM_WINDOW_TYPE");
    testCookie(cc2, connection, "failed setting WM_CLASS");
    testCookie(cc3, connection, "failed setting WM_NAME");
}

void connectionInit() {
    int screenNumber = 0;
    connection = xcb_connect(NULL,&screenNumber);
}

void screenInit() {
    screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
}

void fillGCInit() {
    fillGC = xcb_generate_id(connection);
    uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_GRAPHICS_EXPOSURES;
    uint32_t values[3] = {arguments.bgColor,arguments.bgColor,0};
    xcb_create_gc(connection,fillGC,window,mask,values);
}

void windowInit() {
    uint32_t values[3];
    uint32_t mask = 0;

    window = xcb_generate_id(connection);
    mask = XCB_CW_BACK_PIXEL| XCB_CW_EVENT_MASK;
    values[0] = arguments.bgColor;
    values[1] = XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_EXPOSURE;
    xcb_void_cookie_t windowCookie = xcb_create_window_checked(connection,screen->root_depth,window,screen->root,windowX,windowY,windowWidth,windowHeight,0,XCB_WINDOW_CLASS_INPUT_OUTPUT,screen->root_visual,mask,values);
    testCookie(windowCookie,connection,"can't create window");
}

void mapWindow() {
    xcb_void_cookie_t mapCookie = xcb_map_window_checked (connection, window);
    testCookie(mapCookie,connection,"can't map window");
}

void clearWindow(){
    xcb_rectangle_t rect = {0, 0, windowWidth, windowHeight};
    xcb_poly_fill_rectangle(connection, window, fillGC, 1, &rect);
}

void drawAllText() {

    for (int i = 0; i < numberOfLinesToPrint; i++) {
        drawText(arguments.leftIndent,lineHeight*(i+1)+arguments.topIndent, linesToPrint[i]);
    }
}

int handleEvent(xcb_generic_event_t* event) {
    int shouldFinishAfter = 0;
    switch(event->response_type & ~0x80) { //why this mask?...
        case XCB_EXPOSE: {
            drawAllText();
            break;
        }
        case XCB_KEY_PRESS: {
            xcb_keycode_t keycode = ((xcb_key_press_event_t *)event)->detail;
            if (keycode == 9) {
                shouldFinishAfter = 1;
                break;
            }

            char character = getCharfromKeycode(keycode);
            int selectionResult = selectElement(character);

            if (selectionResult == ELEMENT_SELECTION_OVER) {
                shouldFinishAfter = 1; 
                break;
            }
            
            if (selectionResult == ELEMENT_SELECTION_FALSE) {
                break;
            }
            updateData();
            updateWindowGeometry();
            clearWindow();
            drawAllText();
            break;
        }
        case XCB_BUTTON_PRESS: {
            shouldFinishAfter = 1;
            break;
        }
    } 
    free(event);

    return shouldFinishAfter;
}

void guiStart() {
    windowWidth = arguments.windowWidth;
    connectionInit();
    screenInit();
    windowInit();

    fontGCInit();
    fillGCInit();

    setWindowFlags();

    mapWindow();

    updateData();
    updateWindowGeometry();

    xcb_flush(connection);

    grabKeyboard(10);
}

void guiEnd() {
    //give back control
    releaseKeyboard();

    //disconnect
    xcb_disconnect(connection);
    xcb_flush(connection);

    //i'm sorry. This is needed to avoid hanging interface after levelup+another key pressed at the same time from root menu..
    connection = xcb_connect(NULL,0);
    xcb_disconnect(connection);
    xcb_flush(connection);
}

void guiEventLoop() {
    guiStart();

    int finished = 0;   //return 1 if failure
    xcb_generic_event_t* event;  
    while(!finished && (event = xcb_wait_for_event(connection))) {
        finished = handleEvent(event); //return 1 if an expected exit condition is met
    }

    guiEnd();
}