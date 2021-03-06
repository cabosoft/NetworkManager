//
//  NetworkManager.h
//
//  Created by Robert Ryan on 1/30/14.
//  Copyright (c) 2014 Robert Ryan. All rights reserved.
//
//  This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 International License.
//  http://creativecommons.org/licenses/by-sa/4.0/

#import <Foundation/Foundation.h>
#import "NetworkDataTaskOperation.h"
#import "NetworkDownloadTaskOperation.h"
#import "NetworkUploadTaskOperation.h"

extern NSString * const kNetworkManagerVersion;

@class NetworkManager;

typedef BOOL(^URLSessionDidFinishEventsHandler)(NetworkManager *manager);

typedef void(^DidReceiveChallenge)(NetworkManager *manager,
                                   NSURLAuthenticationChallenge *challenge,
                                   void (^completionHandler)(NSURLSessionAuthChallengeDisposition disposition, NSURLCredential *credential));

typedef void(^DidBecomeInvalidWithError)(NetworkManager *manager,
                                         NSError *error);

typedef void(^DidFinishDownloadingToURL)(NetworkManager *manager,
                                         NSURLSessionDownloadTask *downloadTask,
                                         NSURL *location);

typedef void(^DidCompleteWithError)(NetworkManager *manager,
                                    NSURLSessionTask *task,
                                    NSError *error);

/** Network manager

This creates a `NSURLSession` and manages an collection of `<NetworkTaskOperation>`
objects. The key feature of this class is that you can use it to create
`NSOperation`-based `NSURLSessionTask` objects, which are their own delegates
(generally the session is, inexplicably, the delegate for
`NSURLSessionTaskDelegate` and `NSURLSessionTaskDelegate`. But this
session manager will maintain a collection of `NetworkTaskOperation`
objects, and when it receives task-related delegate calls, it simply passes
the call to the appropriate `<NetworkTaskOperation>` object.

In short, this, in conjunction with `<NetworkTaskOperation>` (and its subclasses),
achieves task-based delegate calls.

##Usage

 1. Create property to hold `NetworkManager`:
 
       @property (nonatomic, strong) NetworkManager *networkManager;

 2. Instantiate `NetworkManager`:

       self.networkManager = [[NetworkManager alloc] init];

 3. Create task and add it to the manager's queue:
 
       NSOperation *operation = [self.networkManager downloadOperationWithURL:url didWriteDataHandler:^(NetworkDownloadTaskOperation *operation, int64_t bytesWritten, int64_t totalBytesWritten, int64_t totalBytesExpectedToWrite) {
            // now update the UI here
       } didFinishDownloadingHandler:^(NetworkDownloadTaskOperation *operation, NSURL *location, NSError *error) {
           // download is done
       }];

       [self.networkManager addOperation:operation];
 
 @note The progress/completion blocks will, by default, be called on the main queue. If you want to use a different GCD queue, specify a non-nil `<completionQueue>` value.

*/

@interface NetworkManager : NSObject

/// ----------------
/// @name Properties
/// ----------------

/** The completion handler to be set by app delegate's `handleEventsForBackgroundURLSession`
 *  and to be called by `URLSessionDidFinishEventsHandler`.
 */
@property (nonatomic, copy) void (^completionHandler)(void);

/** The block that will be called by `URLSessionDidFinishEventsForBackgroundURLSession:`.
 
 This uses the following typedef:
 
    typedef BOOL(^URLSessionDidFinishEventsHandler)(NetworkManager *manager);

 @note If this block calls the completion handler, it should return `NO`, to inform the default `URLSessionDidFinishEvents` method
 that it does not need to call the `completionHandler`. It should also make sure to `nil` the `completionHandler` after it calls it.
 
 If this block does not call the completion handler itself, it should return `YES` to inform
 the default routine that it should call the `completionHandler` and perform the necessary clean-up.
 */
@property (nonatomic, copy) URLSessionDidFinishEventsHandler urlSessionDidFinishEventsHandler;

/** The block that will be called by `URLSession:didReceiveChallenge:completionHandler:`.
 
 This uses the following typedef:
 
    typedef void(^DidReceiveChallenge)(NetworkManager *manager,
                                       NSURLAuthenticationChallenge *challenge,

 */
@property (nonatomic, copy) DidReceiveChallenge didReceiveChallenge;

/** The block that will be called by `URLSession:didBecomeInvalidWithError:`.
 
 This uses the following typedef:
 
    typedef void(^DidBecomeInvalidWithError)(NetworkManager *manager,
                                             NSError *error);

 */
@property (nonatomic, copy) DidBecomeInvalidWithError didBecomeInvalidWithError;

/** The block that will be called by `URLSession:downloadTask:didFinishDownloadingToURL:`.
 Generally we keep the task methods at the task operation class level, but for background
 downloads, we may lose the operations when the app is killed.
 
 This uses the following typedef:
 
    typedef void(^DidFinishDownloadingToURL)(NetworkManager *manager,
                                             NSURLSessionDownloadTask *downloadTask,
                                             NSURL *location);

 */
@property (nonatomic, copy) DidFinishDownloadingToURL didFinishDownloadingToURL;

/** The block that will be called by `URLSession:task:didCompleteWithError:`.
 Generally we keep the task methods at the task operation class level, but for background
 downloads, we may lose the operations when the app is killed.
 
 This uses the following typedef:
 
    typedef void(^DidCompleteWithError)(NetworkManager *manager,
                                        NSURLSessionTask *task,
                                        NSError *error);

 */
@property (nonatomic, copy) DidCompleteWithError didCompleteWithError;

/** Credential to be tried if receive session-level authentication challenge.
 */
@property (nonatomic, strong) NSURLCredential *credential;

/** The GCD queue to which completion/progress blocks will be dispatched. If `nil`, it will use `dispatch_get_main_queue()`.
 *
 * Often, its useful to have the completion blocks run on the main queue (as you're generally updating the UI).
 * But if you're doing something on a background thread that doesn't rely on UI updates (or if performing tests
 * in the absence of a UI), you might want to use a background queue.
 */
@property (nonatomic, strong) dispatch_queue_t completionQueue;


/// ----------------------------
/// @name Initialization methods
/// ----------------------------

/** Create session manager using the supplied NSURLSessionConfiguration
 *
 * @param configuration The NSURLSessionConfiguration for the underlying NSURLSession.
 *
 * @return A session manager.
 */
- (instancetype)initWithSessionConfiguration:(NSURLSessionConfiguration *)configuration;

/** Retrieve (and, if necessary create) background session manager
 *
 * @param identifier Background session identifier
 *
 * @return A session manager.
 */
+ (instancetype) backgroundSessionWithIdentifier:(NSString *)identifier;

/// -----------------------------------------------
/// @name NetworkTaskOperation factory methods
/// -----------------------------------------------

/** Create data task operation.
 *
 * @param request The `NSURLRequest`.
 * @param progressHandler The method that will be called with as the data is being downloaded.
 * @param didCompleteWithDataErrorHandler The block that will be called when the upload is done.
 *
 * @return Returns `NetworkDataTaskOperation`.
 *
 * @note If you supply `progressHandler`, it is assumed that you will take responsibility for
 *       handling the individual data chunks as they come in. If you don't provide this block, this
 *       class will aggregate all of the individual `NSData` objects into one final one for you.
 *
 * @note The progress/completion blocks will, by default, be called on the main queue. If you want
 *       to use a different GCD queue, specify a non-nil `<completionQueue>` value.
 */
- (NetworkDataTaskOperation *)dataOperationWithRequest:(NSURLRequest *)request
                                       progressHandler:(ProgressHandler)progressHandler
                                     completionHandler:(DidCompleteWithDataErrorHandler)didCompleteWithDataErrorHandler;

/** Create data task operation.
 *
 * @param url The NSURL.
 * @param progressHandler The method that will be called with as the data is being downloaded.
 * @param didCompleteWithDataErrorHandler The block that will be called when the upload is done.
 *
 * @return Returns `NetworkDataTaskOperation`.
 *
 * @note If you supply `progressHandler`, it is assumed that you will take responsibility for
 *       handling the individual data chunks as they come in. If you don't provide this block, this
 *       class will aggregate all of the individual `NSData` objects into one final one for you.
 *
 * @note The progress/completion blocks will, by default, be called on the main queue. If you want
 *       to use a different GCD queue, specify a non-nil `<completionQueue>` value.
 */
- (NetworkDataTaskOperation *)dataOperationWithURL:(NSURL *)url
                                   progressHandler:(ProgressHandler)progressHandler
                                 completionHandler:(DidCompleteWithDataErrorHandler)didCompleteWithDataErrorHandler;

/** Create download task operation.
 *
 * @param request The `NSURLRequest`.
 * @param didWriteDataHandler The method that will be called with as the data is being downloaded.
 * @param didFinishDownloadingHandler The block that will be called when the upload is done.
 *
 * @return Returns `NetworkDownloadTaskOperation`.
 *
 * @note The progress/completion blocks will, by default, be called on the main queue. If you want
 *       to use a different GCD queue, specify a non-nil `<completionQueue>` value.
 */

- (NetworkDownloadTaskOperation *)downloadOperationWithRequest:(NSURLRequest *)request
                                           didWriteDataHandler:(DidWriteDataHandler)didWriteDataHandler
                                   didFinishDownloadingHandler:(DidFinishDownloadingHandler)didFinishDownloadingHandler;

/** Create download task operation.
 *
 * @param url The NSURL.
 * @param didWriteDataHandler The method that will be called with as the data is being downloaded.
 * @param didFinishDownloadingHandler The block that will be called when the upload is done.
 *
 * @return Returns `NetworkDownloadTaskOperation`.
 *
 * @note The progress/completion blocks will, by default, be called on the main queue. If you want
 *       to use a different GCD queue, specify a non-nil `<completionQueue>` value.
 */

- (NetworkDownloadTaskOperation *)downloadOperationWithURL:(NSURL *)url
                                       didWriteDataHandler:(DidWriteDataHandler)didWriteDataHandler
                               didFinishDownloadingHandler:(DidFinishDownloadingHandler)didFinishDownloadingHandler;

/** Create download task operation.
 *
 * @param resumeData The `NSData` from `<NetworkDownloadTaskOperation>` method `cancelByProducingResumeData:`.
 * @param didWriteDataHandler The method that will be called with as the data is being downloaded.
 * @param didFinishDownloadingHandler The block that will be called when the upload is done.
 *
 * @return Returns `NetworkDownloadTaskOperation`.
 *
 * @note The progress/completion blocks will, by default, be called on the main queue. If you want
 *       to use a different GCD queue, specify a non-nil `<completionQueue>` value.
 */

- (NetworkDownloadTaskOperation *)downloadOperationWithResumeData:(NSData *)resumeData
                                              didWriteDataHandler:(DidWriteDataHandler)didWriteDataHandler
                                      didFinishDownloadingHandler:(DidFinishDownloadingHandler)didFinishDownloadingHandler;

/** Create upload task operation.
 *
 * @param request The `NSURLRequest`.
 * @param data    The body of the request
 * @param didSendBodyDataHandler The method that will be called with periodic updates while data is being uploaded
 * @param didCompleteWithDataErrorHandler The block that will be called when the upload is done.
 *
 * @return Returns `NetworkUploadTaskOperation`.
 *
 * @note The progress/completion blocks will, by default, be called on the main queue. If you want
 *       to use a different GCD queue, specify a non-nil `<completionQueue>` value.
 */

- (NetworkUploadTaskOperation *)uploadOperationWithRequest:(NSURLRequest *)request
                                                      data:(NSData *)data
                                    didSendBodyDataHandler:(DidSendBodyDataHandler)didSendBodyDataHandler
                           didCompleteWithDataErrorHandler:(DidCompleteWithDataErrorHandler)didCompleteWithDataErrorHandler;

/** Create upload task operation.
 *
 * @param url  The NSURL.
 * @param data The body of the request
 * @param didSendBodyDataHandler The method that will be called with periodic updates while data is being uploaded
 * @param didCompleteWithDataErrorHandler The block that will be called when the upload is done.
 *
 * @return Returns `NetworkUploadTaskOperation`.
 *
 * @note The progress/completion blocks will, by default, be called on the main queue. If you want
 *       to use a different GCD queue, specify a non-nil `<completionQueue>` value.
 */

- (NetworkUploadTaskOperation *)uploadOperationWithURL:(NSURL *)url
                                                  data:(NSData *)data
                                didSendBodyDataHandler:(DidSendBodyDataHandler)didSendBodyDataHandler
                       didCompleteWithDataErrorHandler:(DidCompleteWithDataErrorHandler)didCompleteWithDataErrorHandler;

/** Create upload task operation.
 *
 * @param request The `NSURLRequest`.
 * @param fileURL    The URL of the file to be uploaded
 * @param didSendBodyDataHandler The method that will be called with periodic updates while data is being uploaded
 * @param didCompleteWithDataErrorHandler The block that will be called when the upload is done.
 *
 * @return Returns `NetworkUploadTaskOperation`.
 *
 * @note The progress/completion blocks will, by default, be called on the main queue. If you want
 *       to use a different GCD queue, specify a non-nil `<completionQueue>` value.
 */

- (NetworkUploadTaskOperation *)uploadOperationWithRequest:(NSURLRequest *)request
                                                   fileURL:(NSURL *)fileURL
                                    didSendBodyDataHandler:(DidSendBodyDataHandler)didSendBodyDataHandler
                           didCompleteWithDataErrorHandler:(DidCompleteWithDataErrorHandler)didCompleteWithDataErrorHandler;

/** Create upload task operation.
 *
 * @param url   The NSURL.
 * @param fileURL  The URL of the file to be uploaded
 * @param didSendBodyDataHandler The method that will be called with periodic updates while data is being uploaded
 * @param didCompleteWithDataErrorHandler The block that will be called when the upload is done.
 *
 * @return Returns `NetworkUploadTaskOperation`.
 *
 * @note The progress/completion blocks will, by default, be called on the main queue. If you want
 *       to use a different GCD queue, specify a non-nil `<completionQueue>` value.
 */

- (NetworkUploadTaskOperation *)uploadOperationWithURL:(NSURL *)url
                                               fileURL:(NSURL *)fileURL
                                didSendBodyDataHandler:(DidSendBodyDataHandler)didSendBodyDataHandler
                       didCompleteWithDataErrorHandler:(DidCompleteWithDataErrorHandler)didCompleteWithDataErrorHandler;

/// -----------------------------------------------
/// @name NSOperationQueue utility methods
/// -----------------------------------------------

/** Operation queue for network requests.
 *
 * If you want, you can add operations to the NSURLSessionManager-provided operation queue.
 * This method is provided in case you want to customize the queue or add operations to it yourself.
 *
 * @return An `NSOperationQueue`. This will instantiate a queue if one hadn't already been created.
 */

- (NSOperationQueue *)networkQueue;

/** Add operation.
 *
 * A convenience method to add operation to the network manager's `networkQueue` operation queue.
 *
 * @param operation The operation to be added to the queue.
 */

- (void)addOperation:(NSOperation *)operation;

@end
