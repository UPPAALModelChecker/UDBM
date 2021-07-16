# This file is part of the Ruby binding to the UPPAAL DBM library.
#
# This binding is free software; you can redistribute it
# and/or modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation; either version
# 2 of the License, or (at your option) any later version.
#
# This binding is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public
# License along with this binding; if not, write to the
# Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
# Boston, MA  02110-1301  USA.

require 'thread'
require 'udbm'
require 'udbm-mdi'
require 'udbm-callback'

# Hook to convert Array to Gdk::Color
class Array
  def to_color
    raise 'RGB values expected' if size < 3
    Gdk::Color.new(self[0], self[1], self[2])
  end
end

# Hooks on Gdk::Color
module Gdk
  class Color
    def darker
      to_a.collect { |v| (5*v)/6 }.to_color
    end
    def to_s
      'RGB' + to_a.inspect
    end
  end
end

# Hook to convert Gtk::Allocation to Gdk::Rectangle
module Gtk
  class Allocation
    def to_rectangle
      Gdk::Rectangle.new(x, y, width, height)
    end
  end
end

module UDBM

  # Hooks for 'udbm': wrapper for show & hide.
  class Fed
    # Show this fed under a tab with this title.
    def show(title, labels=nil)
      FedMutex.synchronize { FedWindow.sync_show(self, title, labels) }
    end

    # Hide this fed (if it's shown).
    def hide
      FedMutex.synchronize { FedWindow.sync_hide(self) }
    end

    # For internal use.
    attr_reader :doc
    def attach(doc)
      @doc = doc
    end
  end

  # Hooks for 'udbm-sys'
  class Context
    class Set
      # Wrapper for Fed::show
      def show(name)
        fed.show(name, context.clock_names)
        self
      end

      # Similar but remove the prefix from the names
      def show2(name)
        fed.show(name, context.clock_names.collect { |n| n.sub(/.*\./,'') })
        self
      end

      # Wrapper for Fed::hide
      def hide
        fed.hide
        self
      end

      # Only if Set was defined, otherwise stupid error.
      if Set.public_instance_methods.include?('context=')
        # Proper view update.
        alias_method :_old_new_context, :context=
        def context=(ctx)
          if !context.equal?(ctx)
            doc = fed.doc
            _old_new_context(ctx)
            if doc
              FedMutex.synchronize {
                txt = doc.label.text
                FedWindow.sync_hide(fed)
                FedWindow.sync_show(fed, txt, context.clock_names)
              }
            end
          end
        end
      end
    end
  end

# Gtk::main and the main program run on 2 different threads
# and interact. Obviously, we need to avoid messing up everything.
# By convention method named sync_* have their caller synchronized.
# There is still a race between events generated in Gtk that close
# windows (udbm-mdi) and methods here that search for windows.
  FedMutex = Mutex.new

  # The panel inside the tabs.
  class FedPanel < Gtk::HBox

    @@Border = 10
    @@Arrow = 3
    @@NormalColors = []
    @@DarkerColors = []
    @@White = Gdk::Color.new(0xffff,0xffff,0xffff)
    @@Black = Gdk::Color.new(0,0,0)
    @@LineColor = Gdk::Color.new(0xf400, 0xf400, 0xf400)
    @@XYColor = Gdk::Color.new(0x7890, 0x7890, 0x7890)
    @@StepColors = [ 0xffff, 0xdcdc, 0xbebe, 0xa5a5, 0x9191, 0x7878, 0x5a5a, 0x3c3c, 0 ]

    attr_reader :fed
    attr_reader :dim

    def get_clock_label(labels, i)
      if labels && labels[i-1]
        @mapping[label = labels[i-1].to_s] = i
        label
      else
        i.to_s
      end
    end

    def initialize(fed, labels)
      super(false, 0)
      @fed = fed
      @dim = fed.dim
      @mapping = labels ? Hash.new : nil

      vboxx = Gtk::VBox.new(false, 5)
      vboxy = Gtk::VBox.new(false, 5)
      vboxx.pack_start(Gtk::Label.new('x'), false, false, 0)
      vboxy.pack_start(Gtk::Label.new('y'), false, false, 0)
      label = get_clock_label(labels, 1)
      selectx = [ Gtk::RadioButton.new(label) ]
      selecty = [ Gtk::RadioButton.new(label) ]
      (2..(@dim-1)).each { |i|
        label = get_clock_label(labels, i)
        selectx << Gtk::RadioButton.new(selectx[0], label)
        selecty << Gtk::RadioButton.new(selecty[0], label)
      }
      @x = 1
      if selecty.size > 1
        selecty[1].set_active(true)
        @y = 2
      else
        @y = 1
      end
      selectx.each { |b|
        vboxx.pack_start(b, false, false, 0)
        b.signal_connect('expose_event') { |widget, event| select_expose_event(widget, true) }
      }
      selecty.each { |b|
        vboxy.pack_start(b, false, false, 0)
        b.signal_connect('expose_event') { |widget, event| select_expose_event(widget, false) }
      }

      da = Gtk::DrawingArea.new
      da.set_size_request(100, 100)
      da.modify_bg(Gtk::STATE_NORMAL, @@White)
      da.signal_connect('expose_event') { |widget, event| viewer_expose_event(widget) }

      frame = Gtk::Frame.new
      frame.shadow_type = Gtk::SHADOW_IN
      frame.add(da)

      pack_start(vboxx, false, false, 5)
      pack_start(vboxy, false, false, 5)
      pack_start(frame, true, true, 0)
    end

    def sync_update_fed(fed)
      @fed = fed
      @dim = fed.dim
      sync_redraw
    end

    def set_point(pt)
      @point = pt
      sync_redraw
    end

  private

    def select_expose_event(widget, is_x)
      if widget.active?
        k = @mapping ? @mapping[widget.label] : widget.label.to_i
        k = widget.label.to_i if !k
        if k != (is_x ? @x : @y)
          if is_x
            @x = k
          else
            @y = k
          end
          FedMutex.synchronize { sync_redraw }
        end
      end
    end

    def sync_redraw
      if (window)
        window.invalidate(allocation.to_rectangle, true);
        window.process_updates(true)
      end
    end

    def get_normal_color(i)
      return @@NormalColors[i] if @@NormalColors[i]
      j = i + 1 # otherwise white on white!
      steps = [ 0, 0, 0]
      while j != 0
        steps[j%3] += 1
        j /= 3
      end
      @@NormalColors[i] = steps.collect { |s| @@StepColors[s%@@StepColors.size] }.to_color
    end

    def get_darker_color(i)
      return @@DarkerColors[i] if @@DarkerColors[i]
      @@DarkerColors[i] = get_normal_color(i).darker
    end

    def viewer_expose_event(da)
      width = da.allocation.width-@@Border
      height = da.allocation.height-@@Border
      da.window.draw_segments(style.black_gc, [
        [@@Border, @@Border, @@Border-@@Arrow, @@Border+@@Arrow],
        [@@Border, @@Border, @@Border+@@Arrow, @@Border+@@Arrow],
        [@@Border, @@Border, @@Border, height],
        [@@Border, height, width, height],
        [width, height, width-@@Arrow, height-@@Arrow],
        [width, height, width-@@Arrow, height+@@Arrow]])
    
      gc = Gdk::GC.new(da.window)
      gc.rgb_fg_color = @@XYColor
      layout = Pango::Layout.new(Gdk::Pango.context)
      layout.font_description = Pango::FontDescription.new('Sans 10')
      layout.text = 'y'
      da.window.draw_layout(gc, 0, -5, layout)
      layout.text = 'x'
      da.window.draw_layout(gc, width+1, height-5, layout)
#     layout.text = '0'
#     da.window.draw_layout(gc, 0, height-5, layout)
      layout.font_description = Pango::FontDescription.new('Sans 6')
      gc.function = Gdk::GC::AND
      i = 1
      size = @fed.size
      @fed.drawing(@@Border, width, height, @x, @y).each { |d|
        gc.rgb_fg_color = @@LineColor
        da.window.draw_segments(gc, d[0])
        gc.rgb_fg_color = get_normal_color(size-i)
        da.window.draw_polygon(gc, true, d[1])
        gc.rgb_fg_color = get_darker_color(size-i)
        da.window.draw_segments(gc, d[2])
        d[3].each { |p|
          layout.text = p[2].to_s
          da.window.draw_layout(gc, p[0]+1, p[1], layout);
        }
        i += 1
      }
      if (@point)
        gc.rgb_fg_color = @@Black
        da.window.draw_segments(gc, @fed.point_drawing(@point, @x, @y))
      end
      true
    end

  end

  class FedWindow < Gtk::Window
    @@Instance = nil     # Controller instance.
    @@Mapping = Hash.new # Map name -> document
    attr_reader :tabs

    def FedWindow.init
      if !@@Instance
        @@Instance = Gtk::MDI::Controller.new(FedWindow, :tabs)
        @@Instance.signal_connect('window_removed') { |controller,window,last|
          window.tabs.documents.each { |d| FedWindow.unmap(d) }
        }
      end
    end

    def FedWindow.sync_show(fed, title, labels = nil)
      return fed if fed.dim <= 1
      @@Instance.open_window if @@Instance.windows.size == 0

      if (mdoc = @@Mapping[title])           # There's a fed under title
        if (mdoc.widget.dim != fed.dim)      # - with a different dimension
          FedWindow.sync_clean_up(nil, fed)  # then clean up ref to fed
          FedWindow.sync_hide_title(title)   # and tab with this title
          FedWindow.sync_create(fed, title, labels)  # before creating a new tab.
        else                                 # - with the same dimension
          mdoc.widget.sync_update_fed(fed)   # then update the panel (maybe change fed)
          @@Instance.windows.find { |w| w.tabs.documents.include?(mdoc) }.tabs.focus_document(mdoc)
          fed.attach(mdoc)                   # ensure association,
          FedWindow.sync_clean_up(mdoc, fed) # and clean up any duplicate.
        end
      else                                   # No fed under given title
        @@Instance.windows.each { |w|        # search windows
          w.tabs.documents.each { |d|        # with a document
            if d.widget.fed.object_id == fed.object_id # containing this fed
              w.tabs.focus_document(d)       # then focus it
              d.label.text = title           # and change the title.
              return fed
            end
          }
        }
        FedWindow.sync_create(fed, title, labels)    # We need a new tab with this fed.
      end
      fed
    end

    def FedWindow.sync_hide(fed)
      if fed
        @@Instance.windows.each { |w|        # search windows
          w.tabs.documents.each { |d|        # with a document
            if d.widget.fed.object_id == fed.object_id # containing this fed
              w.tabs.remove_document(d)      # then remove the document
              fed.attach(nil)                # dettach fed & this document
              @@Mapping[d.label.text] = nil  # clear the mapping
              return fed
            end
          }
        }
      end
      fed
    end

    def FedWindow.sync_hide_title(title)
      if (mdoc = @@Mapping[title])           # There's a document with this title
        @@Instance.windows.each { |w|        # then search windows
          w.tabs.documents.each { |d|        # with this document
            if d == mdoc
              w.tabs.remove_document(d)      # to be removed
              d.widget.fed.attach(nil)       # and dettached from its fed.
              @@Mapping[title] = nil         # Clear the mapping.
              return nil
            end
          }
        }
      end
    end

    def FedWindow.sync_get_fed(title)
      (mdoc = @@Mapping[title]) ? mdoc.widget.fed : nil
    end

    def FedWindow.sync_set_point(title,pt)
      if ((mdoc = @@Mapping[title]) != nil)
        mdoc.widget.set_point(pt)
      end
    end

  private

    def FedWindow.sync_create(fed, title, labels)
      if fed.dim > 1 # Otherwise nothing to show & boom
        win = @@Instance.windows[0]
        win.tabs.add_document(doc = Gtk::MDI::Document.new(FedPanel.new(fed, labels), title))
        doc.signal_connect('close') { |d| FedWindow.unmap(d) }
        @@Mapping[title] = doc
        fed.attach(doc)
        win.show
        win.tabs.show_all
        win.tabs.focus_document(doc)
      end
    end

    def FedWindow.sync_clean_up(doc, fed)
      @@Instance.windows.each { |w|
        w.tabs.documents.each { |d|
          if d != doc && d.widget.fed.object_id == fed.object_id
            # other documents than doc have the same fed => remove!
            w.tabs.remove_document(d)
          end
        }
      }
    end

    def FedWindow.unmap(doc)
      FedMutex.synchronize { @@Mapping[doc.label.text] = nil }
    end

    def initialize
      super
      @tabs = Gtk::MDI::Notebook.new
      @tabs.set_tab_pos(Gtk::POS_TOP)
      add(@tabs)
      set_title('Fed viewer')
      set_size_request(320, 240)
    end

  end

  # Hide a tab with a given title.
  def hide(title)
    FedMutex.synchronize { FedWindow.sync_hide_title(title) }
  end

  # Return the Fed associated with a given tab (with this title).
  def get_fed(title)
    FedMutex.synchronize { FedWindow.sync_get_fed(title) }
  end

  # Bootstrap.
  Gtk::init
  FedWindow.init
  Fed.add_change_listener proc { |f,m|
    FedMutex.synchronize {
      if f.doc
        if f.doc.widget.dim != f.dim
          txt = f.doc.label.text      # Read title
          FedWindow.sync_hide(f)      # before hiding
          FedWindow.sync_show(f, txt) # and re-showing a new.
        else
          f.doc.widget.sync_update_fed(f)
        end
      end
    }
  }
  gtk_thread = Thread.new { Gtk::main }
# Uncomment this for Windows but not for Linux.
#  gtk_thread.priority += 1
end

class Array
  def show(title)
    UDBM::FedMutex.synchronize { UDBM::FedWindow.sync_set_point(title,self) }
  end
end
