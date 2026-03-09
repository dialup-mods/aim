<h1 align="center">🌐 AIM: Advanced Injection Module 🌐</h1>
<p align="center">*Best viewed in 800x600*</p>

AIM is the runtime layer of the Dial-Up platform. It sits between `Core` and the
Unreal Engine and is responsible for **safe function interception, task
execution, and engine object access**.

AIM provides an expressive, task-oriented API for observing and interacting with Unreal Engine.

### Example: Goal Speed Anywhere

This plugin displays goal speed regardless of camera or HUD state.
The intent is simple, but existing APIs require coordinating multiple hooks, polling, and manual inference to achieve correct behavior across game modes and states.

In many community frameworks, blocking an engine call also prevents plugins from observing that call.
AIM decouples these concerns, allowing tasks to observe engine behavior even when the original call is suppressed.

### Current community solution
```c++
onLoad() {
	gameWrapper->HookEvent("Function TAGame.Ball_TA.OnHitGoal", std::bind(&GoalSpeedAnywhere::ShowSpeed, this));
	gameWrapper->HookEventWithCaller<BallWrapper>("Function TAGame.Ball_TA.Explode", [this](BallWrapper caller, void* params, std::string eventname) {
		auto explosionParams = (BallExplodeParams*)params;
		if(explosionParams->goal != NULL)
		{
			ShowSpeed();
		}
		// else: The ball exploded for a different reason. Most likely the timer ran out and the ball touched the ground.
	});
	gameWrapper->HookEvent("Function Engine.GameViewportClient.Tick", std::bind(&GoalSpeedAnywhere::GetSpeed, this));
}

void GetSpeed() {
	if(!(*bEnabled) || bShowSpeed) return;
	ServerWrapper server = GetCurrentGameState();
	if(server.IsNull()) return;
	GameSettingPlaylistWrapper playlist = server.GetPlaylist();
	if(playlist.IsNull()) return;
	BallWrapper ball = server.GetBall();
	if(ball.IsNull()) return;

	Speed = ball.GetVelocity().magnitude();

	// The following code is only required for cases where OnHitGoal and Explode events are not sent
	// Currently this can only happen in freeplay with goal scoring turned off in Bakkesmod.
	// The issue in this case is that there is no event to hook in to, so we need to manually check if the ball is inside the goal and was outside before.
	auto isInFreePlay = gameWrapper->IsInFreeplay();
	if(*bGoalScoringIsEnabled || !isInFreePlay) return;

	// This currently only works for "standard" goal areas, i.e. not for goal areas within the pitch (some snow day and rumble maps)
	// or horizontal goal areas (hoops and drop shot).
	static const std::unordered_set<int> excludedPlaylistIds = { 15, 17, 18, 19, 23 }; // Snow Day, Hoops, Rumble, Workshop, Dropshot
	if(excludedPlaylistIds.count(playlist.GetPlaylistId()) > 0) return;

	auto ballRadius = ball.GetRadius();

	ArrayWrapper<GoalWrapper> goalWrappers = server.GetGoals();
	for(auto goalWrapper : goalWrappers)
	{
		auto location = goalWrapper.GetLocation();
		if(!ballIsInsideGoal && abs(ball.GetLocation().Y) >= (abs(location.Y) + ballRadius) )
		{
			ballIsInsideGoal = true;
			DisplaySpeed();
		}
		else if(ballIsInsideGoal && abs(ball.GetLocation().Y) < (abs(location.Y) + ballRadius))
		{
			ballIsInsideGoal = false;
		}
	}
}

```
- Execution order is implicit and scoped to a single engine call

- Correctness depends on multiple hooks firing in the expected order

- Much logic on every tick

- State is shared across callbacks

### The AIM Version

```c++
processEvent->registerTask(
    TaskBuilder()
        .name("GoalSpeedAnywhere")
        .functionName("Function TAGame.Ball_TA.Explode")
        .phase(HookPhase::Post)

        // Ignore non-goal explosions
        .successCondition([](InvocationContext& ctx) {
            return ctx.getParams<BallExplodeParams>()->goal != nullptr;
        })

        .onSuccessCallback([this](InvocationContext& ctx) {
            displayGoalSpeed(
                ctx.getSelf<Ball_TA>()->Velocity().magnitude()
            );
        })

        .build()
);
```
- Tasks are isolated, SEH-wrapped, and failure-tolerant by design; a single misbehaving plugin cannot destabilize the engine or other tasks.
- Does not require logic to run on every tick

### Taking it further

The same task-oriented syntax applies uniformly across engine entry points, including:

- `ProcessEvent`
- `ProcessInternal`
- `CallFunction`

Tasks can express **control flow**, not just callbacks:

- Conditional execution (`runCondition`, `successCondition`)

- Structured retries (`maxAttempts`, `timeoutSeconds`)

- Deterministic ordering (`preStep`, `afterSuccessCallback`)

- Explicit execution control (`nextTick`, `once`)

- Explicit failure handling (`onFailureCallback`)

These features allow complex engine interactions to be expressed locally, without polling loops, global state, or cross-callback coordination.

### Execution Policy Composition

Tasks are first-class objects.

Their execution can be enabled, disabled, or scoped dynamically by other tasks.

```c++
auto task = TaskBuilder()
    .name("Collect Goal Stats")
    .functionName("Function TAGame.Ball_TA.OnHitGoal")
    .callback([] { collect(); })
    .build();

processEvent->registerTask(
    TaskBuilder()
        .name("Enable on match start")
        .functionName("Function TAGame.Game.OnMatchStart")
        .callback([this, task] {
            processEvent_->registerTask(task);
        })
        .build()
);

processEvent->registerTask(
    TaskBuilder()
        .name("Disable on match end")
        .functionName("Function TAGame.Game.OnMatchEnd")
        .callback([this, task] {
            processEvent_->releaseTask(task);
        })
        .build()
);
```
This pattern allows behavior to be scoped, activated, and retired without:

- shared mutable state

- conditional branching inside callbacks

- polling or tick-based logic

- special-case lifecycle flags

Execution rules are composed declaratively, using the same task system.

## Implementation

**Key features:**
- RAII-based resource management
- Type-safe function hooking
- Automatic cleanup on detach
- Cross-DLL safe operation
