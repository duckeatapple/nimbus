//
// Copyright 2011 Jeff Verkoeyen
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import "NIPhotoScrollView.h"

/**
 * The default number of pixels on the side of each photo.
 */
extern const CGFloat NIPhotoAlbumScrollViewDefaultPageHorizontalMargin;

@protocol NIPhotoAlbumScrollViewDataSource;
@protocol NIPhotoAlbumScrollViewDelegate;

/**
 * A paged scroll view that shows a collection of photos.
 *
 * This view provides a light-weight implementation of a photo viewer, complete with
 * pinch-to-zoom and swiping to change photos. It is designed to perform well with
 * large sets of photos and large images that are loaded from either the network or
 * disk.
 *
 * It is intended for this view to be used in conjunction with a view controller that
 * implements the data source protocol and presents any required chrome.
 */
@interface NIPhotoAlbumScrollView : UIView <
  UIScrollViewDelegate,
  NIPhotoScrollViewDelegate
> {
@private
  UIScrollView* _pagingScrollView;

  // Sets of NIPhotoScrollViews
  NSMutableSet* _visiblePages;
  NSMutableSet* _recycledPages;

  // Configurable Properties
  UIImage* _loadingImage;
  CGFloat _pageHorizontalMargin;
  BOOL _zoomingIsEnabled;

  // State Information
  NSInteger _firstVisiblePageIndexBeforeRotation;
  CGFloat   _percentScrolledIntoFirstVisiblePage;
  BOOL      _isModifyingContentOffset;
  NSInteger _currentCenterPhotoIndex;

  // Cached Data Source Information
  NSInteger _numberOfPages;

  id<NIPhotoAlbumScrollViewDataSource> _dataSource;
  id<NIPhotoAlbumScrollViewDelegate> _delegate;
}

/**
 * @name Configuring Presentation
 * @{
 */
#pragma mark Configuring Presentation

/**
 * An image that is displayed while the photo is loading.
 *
 * This photo will be presented if no image is returned in the data source's implementation
 * of photoAlbumScrollView:photoAtIndex:photoSize:isLoading:.
 *
 * Zooming is disabled when showing a loading image, regardless of the state of zoomingIsEnabled.
 *
 * By default this is nil.
 */
@property (nonatomic, readwrite, retain) UIImage* loadingImage;

/**
 * The number of pixels on either side of each photo page.
 *
 * The space between each photo will be 2x this value.
 *
 * By default this is NIPhotoAlbumScrollViewDefaultPageHorizontalMargin.
 */
@property (nonatomic, readwrite, assign) CGFloat pageHorizontalMargin;

/**@}*/// End of Configuring Presentation


/**
 * @name Configuring Functionality
 * @{
 */
#pragma mark Configuring Functionality

/**
 * Whether zooming is enabled or not.
 *
 * Regardless of whether this is enabled, only original-sized images will be zoomable.
 * This is because we often don't know how large the final image is so we can't
 * calculate min and max zoom amounts correctly.
 *
 * By default this is YES.
 */
@property (nonatomic, readwrite, assign) BOOL zoomingIsEnabled;

/**@}*/// End of Configuring Functionality


/**
 * @name Data Source
 * @{
 */
#pragma mark Data Source

/**
 * The data source for this photo album view.
 */
@property (nonatomic, readwrite, assign) id<NIPhotoAlbumScrollViewDataSource> dataSource;

/**
 * Force the view to reload its data by asking the data source for information.
 *
 * This must be called at least once after dataSource has been set in order for the view
 * to gather any presentable information.
 *
 * This method is expensive. It will reset the state of the view and remove all existing
 * pages before requesting the new information from the data source.
 */
- (void)reloadData;

/**@}*/// End of Data Source


/**
 * The delegate for this photo album view.
 */
@property (nonatomic, readwrite, assign) id<NIPhotoAlbumScrollViewDelegate> delegate;


/**
 * The current center photo index.
 */
@property (nonatomic, readonly, assign) NSInteger currentCenterPhotoIndex;

/**
 * The total number of photos in this photo album view, as gathered from the data source.
 */
@property (nonatomic, readonly, assign) NSInteger numberOfPhotos;

/**
 * Returns YES if there is a next photo.
 */
- (BOOL)hasNext;

/**
 * Returns YES if there is a previous photo.
 */
- (BOOL)hasPrevious;

/**
 * Move to the next photo if there is one.
 */
- (void)moveToNextAnimated:(BOOL)animated;

/**
 * Move to the previous photo if there is one.
 */
- (void)moveToPreviousAnimated:(BOOL)animated;

/**
 * @name Notifying the View of Loaded Photos
 * @{
 */
#pragma mark Notifying the View of Loaded Photos

/**
 * Notify the scroll view that a photo has been loaded at a given index.
 *
 * You should notify the completed loading of thumbnails as well. Calling this method
 * is fairly lightweight and will only update the images of the visible pages.
 *
 * The photo at the given index will only be replaced with the given image if photoSize
 * is of a higher quality than the currently-displayed photo size.
 */
- (void)didLoadPhoto: (UIImage *)image
             atIndex: (NSInteger)photoIndex
           photoSize: (NIPhotoScrollViewPhotoSize)photoSize;

/**@}*/// End of Notifying the View of Loaded Photos


/**
 * @name Rotating the Scroll View
 * @{
 */
#pragma mark Rotating the Scroll View

/**
 * Stores the current state of the scroll view in preparation for rotation.
 *
 * This must be called in conjunction with willAnimateRotationToInterfaceOrientation:duration:
 * in the methods by the same name from the view controller containing this view.
 */
- (void)willRotateToInterfaceOrientation: (UIInterfaceOrientation)toInterfaceOrientation
                                duration: (NSTimeInterval)duration;

/**
 * Updates the frame of the scroll view while maintaining the current visible page's state.
 */
- (void)willAnimateRotationToInterfaceOrientation: (UIInterfaceOrientation)toInterfaceOrientation
                                         duration: (NSTimeInterval)duration;

/**@}*/// End of Rotating the Scroll View


@end


@protocol NIPhotoAlbumScrollViewDataSource <NSObject>

@required

/**
 * Fetches the total number of photos in the scroll view.
 *
 * The value returned in this method will be cached until reloadData is called again.
 */
- (NSInteger)numberOfPhotosInPhotoScrollView:(NIPhotoAlbumScrollView *)photoScrollView;

/**
 * Fetches the highest-quality image available for the photo at the given index.
 *
 * Your goal should be to make this implementation return as fast as possible. Avoid
 * hitting the disk or blocking on a network request. Aim to load images asynchronously.
 *
 * If you already have the highest-quality image in memory (like in an NIImageMemoryCache),
 * then you can simply return the image and set photoSize to be
 * NIPhotoScrollViewPhotoSizeOriginal.
 *
 * If the highest-quality image is not available when this method is called you should
 * spin off an asynchronous operation to load the image and set isLoading to YES.
 *
 * If you have a thumbnail in memory but not the full-size image yet, then you should return
 * the thumbnail, set isLoading to YES, and set photoSize to NIPhotoScrollViewPhotoSizeThumbnail.
 *
 * Once the high-quality image finishes loading, call didLoadPhoto:atIndex:photoSize: with
 * the image.
 *
 * This method will be called to prefetch the next and previous photos in the scroll view.
 * The currently displayed photo will always be requested first.
 *
 *      @attention The photo scroll view does not hold onto the UIImages for very long at all.
 *                 It is up to the controller to decide on an adequate caching policy to ensure
 *                 that images are kept in memory through the life of the photo album.
 *                 In your implementation of the data source you should prioritize thumbnails
 *                 being kept in memory over full-size images. When a memory warning is received,
 *                 the original photos should be relinquished from memory first.
 */
- (UIImage *)photoAlbumScrollView: (NIPhotoAlbumScrollView *)photoAlbumScrollView
                     photoAtIndex: (NSInteger)photoIndex
                        photoSize: (NIPhotoScrollViewPhotoSize *)photoSize
                        isLoading: (BOOL *)isLoading;

@optional

/**
 * Called when you should cancel any asynchronous loading requests for the given photo.
 *
 * When a photo is not immediately visible this method is called to allow the data
 * source to minimize the number of active asynchronous operations in place.
 *
 * This method is optional, though recommended because it focuses the device's processing
 * power on the most immediately accessible photos.
 */
- (void)photoAlbumScrollView: (NIPhotoAlbumScrollView *)photoAlbumScrollView
     stopLoadingPhotoAtIndex: (NSInteger)photoIndex;

@end


@protocol NIPhotoAlbumScrollViewDelegate <NSObject>

@optional

/**
 * The user is scrolling between two photos.
 */
- (void)photoAlbumScrollViewDidScroll:(NIPhotoAlbumScrollView *)photoAlbumScrollView;

/**
 * The user double-tapped to zoom in or out.
 */
- (void)photoAlbumScrollView: (NIPhotoAlbumScrollView *)photoAlbumScrollView
                   didZoomIn: (BOOL)didZoomIn;

/**
 * The current page has changed.
 */
- (void)photoAlbumScrollViewDidChangePages:(NIPhotoAlbumScrollView *)photoAlbumScrollView;

@end
