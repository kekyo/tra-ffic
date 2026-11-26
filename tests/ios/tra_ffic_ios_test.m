/* tra-ffic - Universal asynchronous full-duplex marshaling helper library.
 * Copyright (c) Kouji Matsui (@kekyo@mi.kekyo.net)
 * Under MIT.
 * https://github.com/kekyo/tra-ffic/
 */

#import <UIKit/UIKit.h>
#include <stdio.h>
#include <stdlib.h>

extern int tra_ffic_ios_regression_main(void);

@interface TraFficAppDelegate : UIResponder <UIApplicationDelegate>
@end

@implementation TraFficAppDelegate
@end

@interface TraFficSceneDelegate : UIResponder <UIWindowSceneDelegate>
@end

@implementation TraFficSceneDelegate
- (void)sceneDidBecomeActive:(UIScene *)scene {
  (void)scene;
  int status = tra_ffic_ios_regression_main();
  puts(status == 0 ? "tra_ffic_ios_test: PASS" : "tra_ffic_ios_test: FAIL");
  fflush(NULL);
  _Exit(status);
}
@end

int main(int argc, char **argv) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, NSStringFromClass([TraFficAppDelegate class]));
  }
}
