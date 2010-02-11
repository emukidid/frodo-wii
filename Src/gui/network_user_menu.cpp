#include "gui.hh"
#include "menu.hh"
#include "help_box.hh"
#include "status_bar.hh"
#include "data_store.hh"

#include <Network.h>

class NetworkUserView;

class PeerInfo
{
public:
	PeerInfo(const char *name, int scr_key)
	{
		this->name = (const char*)xstrdup(name);
		this->scr = NULL;
		this->region = 0;
		this->scr_key = scr_key;
	}

	~PeerInfo()
	{
		SDL_FreeSurface(this->scr);
		free((void*)this->name);
	}

	SDL_Surface *getScreenshot()
	{
		struct ds_data *data;

		if (this->scr)
			return this->scr;
		/* Look it up and store it when done */
		data = DataStore::ds->getData(this->scr_key);
		this->scr = sdl_surface_from_data(data->data, data->sz);

		return this->scr;
	}

	const char *getRegion()
	{
		switch (this->region)
		{
		case 1: return "Europe";
		case 2: return "Africa";
		case 3: return "North America";
		case 4: return "South America";
		case 5: return "Asia";
		case 6: return "Australia";
		case 7: return "Antartica"; // Likely, yes
		default:
			break;
		}

		return "Unknown";
	}

	SDL_Surface *scr;
	const char *name;
	int region;
	int scr_key;
};

class PeerInfoBox : public Menu
{
public:
	PeerInfoBox(Font *font) : Menu(font)
	{
		this->pi = NULL;
		memset(this->pi_messages, 0, sizeof(this->pi_messages));
		this->setSelectedBackground(NULL, NULL, NULL, NULL, NULL, NULL);
	}

	void setPeerInfo(PeerInfo *pi)
	{
		this->pi = pi;
		this->updateMessages();
	}


	virtual void selectCallback(int which) { }
	virtual void hoverCallback(int which) { }
	virtual void escapeCallback(int which) { }

	void draw(SDL_Surface *where, int x, int y, int w, int h)
	{
		SDL_Surface *screenshot;
		SDL_Rect dst;

		if (!this->pi)
			return;

		screenshot = this->pi->getScreenshot();
		if (!screenshot)
		{
			Menu::draw(where, x, y + 10, w, h - 10);
			return;
		}

		/* Blit the screenshot */
		dst = (SDL_Rect){x + w / 2 - screenshot->w / 2, y, w, h};
		SDL_BlitSurface(screenshot, NULL, where, &dst);

		Menu::draw(where, x, y + screenshot->h + 10, w, h - screenshot->h - 10);

	}

	void updateMessages()
	{
		this->setText(NULL);
		memset(this->pi_messages, 0, sizeof(this->pi_messages));

		this->pi_messages[0] = "Name:";
		this->pi_messages[1] = " ";
		this->pi_messages[2] = "Region:";
		this->pi_messages[3] = " ";
		this->pi_messages[4] = "Vobb:";
		this->pi_messages[5] = " ";

		if (this->pi)
		{
			this->pi_messages[1] = this->pi->name;
			this->pi_messages[3] = this->pi->getRegion();
			this->pi_messages[5] = " ";
		}

		this->setText(this->pi_messages);
	}

	const char *pi_messages[8];
	PeerInfo *pi;
};

class NetworkUserMenu : public Menu
{
	friend class NetworkUserView;

public:
	NetworkUserMenu(Font *font, PeerInfoBox *infoBox) : Menu(font)
	{
		this->setText(NULL);
		this->peers = NULL;
		this->n_peers = 0;
		this->infoBox = infoBox;
	}

	~NetworkUserMenu()
	{
		this->freePeers();
	}

	void setPeers(NetworkUpdateListPeers *peerList)
	{
		NetworkUpdatePeerInfo *ps = peerList->peers;
		const char **messages;

		if (ps == NULL)
			return;

		this->freePeers();
		messages = (const char **)xmalloc( (peerList->n_peers + 1) *
				sizeof(const char*));

		for (unsigned i = 0; i < peerList->n_peers; i++)
		{
			messages[i] = (const char*)xstrdup((char*)ps->name);
			this->peers[i] = new PeerInfo(messages[i], ps->screenshot_key);
		}
		this->setText(messages);
		free((void*)messages);
	}

	virtual void selectCallback(int which)
	{
		Gui::gui->popView();
	}

	virtual void hoverCallback(int which)
	{
		panic_if(which >= (int)this->n_peers,
				"Which is impossibly large: %d vs %d\n",
				which, this->n_peers);

		this->infoBox->setPeerInfo(this->peers[which]);
	}

	virtual void escapeCallback(int which)
	{
		Gui::gui->popView();
	}

private:
	void freePeers()
	{
		for (unsigned i = 0; i < this->n_peers; i++)
			delete this->peers[i];
		free(this->peers);
	}

	PeerInfo **peers;
	PeerInfoBox *infoBox;
	unsigned int n_peers;
};


class NetworkUserView : public GuiView
{
public:
	NetworkUserView() : GuiView()
	{
		this->peerInfo = new PeerInfoBox(Gui::gui->default_font);
		this->menu = new NetworkUserMenu(Gui::gui->default_font, this->peerInfo);
	}

	~NetworkUserView()
	{
		delete this->menu;
	}

	void runLogic()
	{
		this->menu->runLogic();
	}

	void pushEvent(SDL_Event *ev)
	{
		this->menu->pushEvent(ev);
	}

	void draw(SDL_Surface *where)
	{
		 SDL_Rect dst;

		 /* Blit the backgrounds */
		 dst = (SDL_Rect){20,45,300,400};
		 SDL_BlitSurface(Gui::gui->main_menu_bg, NULL, where, &dst);

		 dst = (SDL_Rect){350,13,0,0};
		 SDL_BlitSurface(Gui::gui->disc_info, NULL, where, &dst);

		 this->menu->draw(where, 50, 70, 280, 375);
		 this->peerInfo->draw(where, 360, 55, 262, 447);
	}

protected:
	NetworkUserMenu *menu;
	PeerInfoBox *peerInfo;
};