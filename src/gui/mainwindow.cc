#include "globals.h"
#include "ui.h"
#include "uipp.h"
#include "gui.h"
#include "threads.h"
#include "proto.h"
#include <unistd.h>

using namespace std::placeholders;

static uiDrawStrokeParams STROKE = {
	uiDrawLineCapFlat,
	uiDrawLineJoinMiter,
	0.5,
	1.0,
	nullptr,
	0,
	0.0
};

static uiDrawBrush WHITE = { uiDrawBrushTypeSolid, 1.0, 0.0, 1.0, 1.0 };
static uiDrawBrush BLACK = { uiDrawBrushTypeSolid, 0.0, 0.0, 0.0, 1.0 };


class MainApp : public UIAllocator, public Showable
{
public:
	MainApp()
	{
		auto configs = findConfigs();
		_window = make<UIWindow>("FluxEngine", 640, 480)
			->setOnClose(_onclose_cb)
			->setChild(
				make<UIVBox>()
					->add(make<UIHBox>()
						->add(make<UICombo<ConfigProto*>>()->setOptions(configs))
						->add(_busyButton = make<UIButton>("busy button"))
						->add(make<UIButton>("press me!"))
					)
					->add(make<UIArea>()
						->setOnDraw(_onredraw_cb)
						->setStretchy(true)
						->disable())
					->add(make<UIButton>("press me again!"))
			);

		UIStartAppThread(_appthread_cb, _appthreadexit_cb);
	}

	void show()
	{
		_window->show();
	}

	void hide()
	{
		_window->hide();
	}

private:
	std::function<int()> _onclose_cb =
		[&] {
			uiQuit();
			return 1;
		};

	std::function<void(uiAreaDrawParams*)> _onredraw_cb =
		[&](auto p) {
			UIPath(p).rectangle(0, 0, p->AreaWidth, p->AreaHeight).fill(WHITE);
			UIPath(p).begin(0, 0).lineTo(p->AreaWidth, p->AreaHeight).end().stroke(BLACK, STROKE);
		};

	std::function<void()> _appthread_cb =
		[&] {
			UIRunOnUIThread([&] { _busyButton->setText("Busy"); });
			sleep(5);
		};

	std::function<void()> _appthreadexit_cb =
		[&] {
			_busyButton->setText("Not busy");
		};

private:
	std::vector<std::pair<std::string, ConfigProto*>> findConfigs()
	{
		std::vector<std::pair<std::string, ConfigProto*>> configs;
		for (const auto& it : formats)
		{
			auto& config = _configs.emplace_back();
			if (!config.ParseFromString(it.second))
				Error() << "couldn't load built-in config proto";

			configs.push_back(std::make_pair(config.comment(), &config));
		}
		return configs;
	}

private:
	UIWindow* _window;
	UIButton* _busyButton;
	std::vector<ConfigProto> _configs;
};

std::unique_ptr<Showable> createMainApp()
{
	return std::make_unique<MainApp>();
}

