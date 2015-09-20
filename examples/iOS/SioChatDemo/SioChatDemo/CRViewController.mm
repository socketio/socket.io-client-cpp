//
//  CRViewController.m
//  ChatRoom
//
//  Created by Melo Yao on 3/30/15.
//

#import "CRViewController.h"
#include "sio_client.h"

typedef enum MessageFlag
{
    Message_System,
    Message_Other,
    Message_You
};

@interface MessageItem : NSObject
@property NSString* message;
@property MessageFlag flag; //0 system info, 1 other message, 2 your message
@end

@implementation MessageItem

@end

@interface CRViewController ()<UITableViewDataSource,UITableViewDelegate,NSURLConnectionDelegate,NSURLConnectionDataDelegate,UITextFieldDelegate>
{
    sio::client *_io;
    NSMutableArray *_receivedMessage;
    NSMutableSet *_typingUsers;
    NSString* _name;
    NSInteger _userCount;
    NSTimer* _inputTimer;
}
@property (weak, nonatomic) IBOutlet UILabel *infoLabel;
@property (weak, nonatomic) IBOutlet UILabel *typingLabel;

@property (strong, nonatomic) IBOutlet UIView *loginPage;
@property (weak, nonatomic) IBOutlet UITextField *nickName;
- (IBAction)onSend:(id)sender;

@property (weak, nonatomic) IBOutlet UITableView *tableView;
@property (weak, nonatomic) IBOutlet UIButton *sendBtn;
@property (weak, nonatomic) IBOutlet UITextField *messageField;
@property (weak, nonatomic) IBOutlet UIView *messageArea;

-(void)onNewMessage:(NSString*) message from:(NSString*) name;

-(void)onUserJoined:(NSString*)user participants:(NSInteger) num;

-(void)onUserLeft:(NSString*) user participants:(NSInteger) num;

-(void)onUserTyping:(NSString*) user;

-(void)onUserStopTyping:(NSString*) user;

-(void)onLogin:(NSInteger) numParticipants;

-(void)onConnected;

-(void)onDisconnected;

-(void) updateUser:(NSString*)user count:(NSInteger) num joinOrLeft:(BOOL) isJoin;

@end

using namespace std;

using namespace sio;

void OnNewMessage(CFTypeRef ctrl,string const& name,sio::message::ptr const& data,bool needACK,sio::message::list ackResp)
{
    if(data->get_flag() == message::flag_object)
    {
        NSString* msg = [NSString stringWithUTF8String:data->get_map()["message"]->get_string().data()];
        NSString* user = [NSString stringWithUTF8String:data->get_map()["username"]->get_string().data()];
        dispatch_async(dispatch_get_main_queue(), ^{
            [((__bridge CRViewController*)ctrl) onNewMessage:msg from:user];
        });
    }
   
}

void OnTyping(CFTypeRef ctrl,string const& name,sio::message::ptr const& data,bool needACK,sio::message::list ackResp)
{
    if(data->get_flag() == message::flag_object)
    {
        NSString* user = [NSString stringWithUTF8String:data->get_map()["username"]->get_string().data()];
        dispatch_async(dispatch_get_main_queue(), ^{
            [((__bridge CRViewController*)ctrl) onUserTyping:user];
        });
    }
}

void OnStopTyping(CFTypeRef ctrl,string const& name,sio::message::ptr const& data,bool needACK,sio::message::list ackResp)
{
    if(data->get_flag() == message::flag_object)
    {
        NSString* user = [NSString stringWithUTF8String:data->get_map()["username"]->get_string().data()];
        dispatch_async(dispatch_get_main_queue(), ^{
            [((__bridge CRViewController*)ctrl) onUserStopTyping:user];
        });
    }
}

void OnUserJoined(CFTypeRef ctrl, string const& name, sio::message::ptr const& data, bool needACK, sio::message::list ackResp)
{
    if(data->get_flag() == message::flag_object)
    {
        NSString* user = [NSString stringWithUTF8String:data->get_map()["username"]->get_string().data()];
        NSInteger num = data->get_map()["numUsers"]->get_int();
        dispatch_async(dispatch_get_main_queue(), ^{
            [((__bridge CRViewController*)ctrl) onUserJoined:user participants:num];
        });
    }
}

void OnUserLeft(CFTypeRef ctrl, string const& name, sio::message::ptr const& data, bool needACK, sio::message::list ackResp)
{
    if(data->get_flag() == message::flag_object)
    {
        NSString* user = [NSString stringWithUTF8String:data->get_map()["username"]->get_string().data()];
        NSInteger num = data->get_map()["numUsers"]->get_int();
        dispatch_async(dispatch_get_main_queue(), ^{
            [((__bridge CRViewController*)ctrl) onUserLeft:user participants:num];
        });
    }
}


void OnLogin(CFTypeRef ctrl, string const& name, sio::message::ptr const& data, bool needACK, sio::message::list ackResp)
{
    if(data->get_flag() == message::flag_object)
    {
        NSInteger num = data->get_map()["numUsers"]->get_int();
        dispatch_async(dispatch_get_main_queue(), ^{
            [((__bridge CRViewController*)ctrl) onLogin:num];
        });
    }
}

void OnConnected(CFTypeRef ctrl,std::string nsp)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [((__bridge CRViewController*)ctrl) onConnected];
    });
}

void OnFailed(CFTypeRef ctrl)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [((__bridge CRViewController*)ctrl) onDisconnected];
    });
}

void OnClose(CFTypeRef ctrl,sio::client::close_reason const& reason)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [((__bridge CRViewController*)ctrl) onDisconnected];
    });
}


@implementation CRViewController

-(void)awakeFromNib
{
    _receivedMessage = [NSMutableArray array];
    _typingUsers = [NSMutableSet set];
    _io = new sio::client();
    [self.loginPage setFrame:self.view.bounds];
    [self.view addSubview:self.loginPage];
    self.nickName.enabled = YES;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
}

-(void)viewWillAppear:(BOOL)animated
{
    _io->set_socket_open_listener(std::bind(&OnConnected, (__bridge CFTypeRef)self,std::placeholders::_1));
    _io->set_close_listener(std::bind(&OnClose, (__bridge CFTypeRef)self, std::placeholders::_1));
    _io->set_fail_listener(std::bind(&OnFailed, (__bridge CFTypeRef)self));
}

-(void)keyboardWillShow:(NSNotification*)notification
{
    CGFloat height = [[notification.userInfo objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue].size.height;
    // Animate the current view out of the way
    [UIView animateWithDuration:0.35 animations:^{
        self.messageArea.transform = CGAffineTransformMakeTranslation(0, -height);
    }];
}


-(void)keyboardWillHide {
    [UIView animateWithDuration:0.35 animations:^{
         self.messageArea.transform = CGAffineTransformIdentity;
    }];
}

-(void)viewDidDisappear:(BOOL)animated
{
    _io->socket()->off_all();
    _io->set_open_listener(nullptr);
    _io->set_close_listener(nullptr);
    _io->close();
}

-(void)onNewMessage:(NSString*) message from:(NSString*) name
{
    MessageItem *item = [[MessageItem alloc] init];

    item.flag = [name isEqualToString:_name]?Message_You:Message_Other;
    item.message = item.flag == Message_You? [NSString stringWithFormat:@"%@:%@",message,name]:[NSString stringWithFormat:@"%@:%@",name,message];
    [_receivedMessage addObject:item];
    [_tableView reloadData];
    
    if(![_messageField isFirstResponder])
    {
        [_tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:[_receivedMessage count]-1 inSection:0] atScrollPosition:UITableViewScrollPositionBottom animated:YES];
    }
}

-(void)onUserJoined:(NSString*)user participants:(NSInteger) num
{
    _userCount = num;
    [self updateUser:user count:num joinOrLeft:YES];
}

-(void)onUserLeft:(NSString*) user participants:(NSInteger) num
{
    [_typingUsers removeObject:user];//protective removal.
    [self updateTyping];
    _userCount = num;
    [self updateUser:user count:num joinOrLeft:NO];
}

-(void)onUserTyping:(NSString*) user
{
    [_typingUsers addObject:user];
    [self updateTyping];
}

-(void)onUserStopTyping:(NSString*) user
{
    [_typingUsers removeObject:user];
    [self updateTyping];
}

-(void)onLogin:(NSInteger) numParticipants
{
    _name = _nickName.text;
    _userCount = numParticipants;
    [self.loginPage removeFromSuperview];
    
    [self updateUser:nil count:_userCount joinOrLeft:YES];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(keyboardWillShow:)
                                                 name:UIKeyboardWillShowNotification
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(keyboardWillHide)
                                                 name:UIKeyboardWillHideNotification
                                               object:nil];
    
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)onSend:(id)sender {
    if ([_messageField.text length]>0 && [_name length]>0) {
        _io->socket()->emit("new message",[_messageField.text UTF8String]);
        MessageItem *item = [[MessageItem alloc] init];
        
        item.flag = Message_You;
        item.message = [NSString stringWithFormat:@"%@:You",_messageField.text];
        [_receivedMessage addObject:item];
        [_tableView reloadData];
        [_tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:[_receivedMessage count]-1 inSection:0] atScrollPosition:UITableViewScrollPositionBottom animated:YES];
    }
    
    self.messageField.text = nil;
    [self.messageField resignFirstResponder];
}

-(void)onConnected
{
    _io->socket()->emit("add user", [self.nickName.text UTF8String]);
}

-(void)onDisconnected
{
    if([self.loginPage superview] == nil)
    {
        [self.view addSubview:self.loginPage];
    }
    self.nickName.enabled = YES;
    
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:UIKeyboardWillShowNotification
                                                  object:nil];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:UIKeyboardWillHideNotification
                                                  object:nil];
}

-(void) updateUser:(NSString*)user count:(NSInteger) num joinOrLeft:(BOOL) isJoin
{
    _userCount = num;
    MessageItem *item = [[MessageItem alloc] init];
    
    item.flag = Message_System;
    if (user) {
        item.message = [NSString stringWithFormat:@"%@ %@\n%@",user,isJoin?@"joined":@"left",num==1?@"there's 1 participant":[NSString stringWithFormat:@"there are %ld participants",num]];
    }
    else
    {
        item.message = [NSString stringWithFormat:@"Welcome to Socket.IO Chat-\n%@",num==1?@"there's 1 participant":[NSString stringWithFormat:@"there are %ld participants",num]];
    }

    [_receivedMessage addObject:item];
    [_tableView reloadData];
    if(![_messageField isFirstResponder])
    {
        [_tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:[_receivedMessage count]-1 inSection:0] atScrollPosition:UITableViewScrollPositionBottom animated:YES];
    }
}

-(void) inputTimeout
{
    _inputTimer = nil;
    _io->socket()->emit("stop typing", "");
}

-(BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
    if(textField == self.messageField)
    {
        if(_inputTimer.valid)
        {
            [_inputTimer setFireDate:[NSDate dateWithTimeIntervalSinceNow:1.0]];
        }
        else
        {
            _io->socket()->emit("typing", "");
            _inputTimer = [NSTimer scheduledTimerWithTimeInterval:1.0 target:self selector:@selector(inputTimeout) userInfo:nil repeats:NO];
        }
    }
    return YES;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return [_receivedMessage count];
}

// Row display. Implementers should *always* try to reuse cells by setting each cell's reuseIdentifier and querying for available reusable cells with dequeueReusableCellWithIdentifier:
// Cell gets various attributes set automatically based on table (separators) and data source (accessory views, editing controls)

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell* cell = [tableView dequeueReusableCellWithIdentifier:@"Msg"];
    MessageItem* item = [_receivedMessage objectAtIndex:indexPath.row];
    cell.textLabel.text = item.message;
    switch (item.flag) {
        case Message_System:
            cell.textLabel.textAlignment = NSTextAlignmentCenter;
            [cell.textLabel setFont:[UIFont fontWithName:[cell.textLabel.font fontName] size:12]];
            break;
        case Message_Other:
            [cell.textLabel setFont:[UIFont fontWithName:[cell.textLabel.font fontName] size:15]];
            cell.textLabel.textAlignment = NSTextAlignmentLeft;
            break;
        case Message_You:
            [cell.textLabel setFont:[UIFont fontWithName:[cell.textLabel.font fontName] size:15]];
            cell.textLabel.textAlignment = NSTextAlignmentRight;
            break;
        default:
            break;
    }
    return cell;
}

-(void)scrollViewDidScroll:(UIScrollView *)scrollView
{
    if ([self.messageField isFirstResponder]) {
        [self.messageField resignFirstResponder];
    }
}

-(BOOL)textFieldShouldReturn:(UITextField *)textField
{
    if (textField == self.nickName) {
        if ([self.nickName.text length] > 0) {
            
            using std::placeholders::_1;
            using std::placeholders::_2;
            using std::placeholders::_3;
            using std::placeholders::_4;
            socket::ptr socket = _io->socket();
            
            socket->on("new message", std::bind(&OnNewMessage, (__bridge CFTypeRef)self, _1,_2,_3,_4));
            socket->on("typing", std::bind(&OnTyping, (__bridge CFTypeRef)self, _1,_2,_3,_4));
            socket->on("stop typing", std::bind(&OnStopTyping, (__bridge CFTypeRef)self, _1,_2,_3,_4));
            socket->on("user joined", std::bind(&OnUserJoined, (__bridge CFTypeRef)self, _1,_2,_3,_4));
            socket->on("user left", std::bind(&OnUserLeft, (__bridge CFTypeRef)self, _1,_2,_3,_4));
            socket->on("login", std::bind(&OnLogin, (__bridge CFTypeRef)self, _1,_2,_3,_4));
            _io->connect("ws://localhost:3000");
            self.nickName.enabled = NO;
        }
    }
    else if(textField == self.messageField)
    {
        [self onSend:textField];
    }
    return YES;
}

-(void)updateTyping
{
    NSString* typingMsg = nil;
    NSString* name = [_typingUsers anyObject];
    if (name) {
        if([_typingUsers count]>1)
        {
            typingMsg = [NSString stringWithFormat:@"%@ and %ld more are typing",name,[_typingUsers count]];
        }
        else
        {
            typingMsg =[NSString stringWithFormat:@"%@ is typing",name];
        }
    }
    self.typingLabel.text = typingMsg;
}

- (void)dealloc
{
    delete _io;
}

@end
