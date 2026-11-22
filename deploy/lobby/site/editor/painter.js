'use strict';

// Terrain colours match minimapUnblocked / minimapBlocked in config/terrains.json
const TERRAINS = [
	{ id: 'dirt', letter: 'd', color: [82, 56, 8], blocked: [57, 40, 8], en: 'Dirt', ru: 'Грязь' },
	{ id: 'sand', letter: 's', color: [222, 207, 140], blocked: [165, 158, 107], en: 'Sand', ru: 'Песок' },
	{ id: 'grass', letter: 'g', color: [0, 65, 0], blocked: [0, 48, 0], en: 'Grass', ru: 'Трава' },
	{ id: 'snow', letter: 'n', color: [181, 199, 198], blocked: [140, 158, 156], en: 'Snow', ru: 'Снег' },
	{ id: 'swamp', letter: 'm', color: [74, 134, 107], blocked: [33, 89, 66], en: 'Swamp', ru: 'Болото' },
	{ id: 'rough', letter: 'r', color: [132, 113, 49], blocked: [99, 81, 33], en: 'Rough', ru: 'Бездорожье' },
	{ id: 'subterra', letter: 'u', color: [132, 48, 0], blocked: [90, 8, 0], en: 'Subterranean', ru: 'Подземелье' },
	{ id: 'lava', letter: 'l', color: [74, 73, 74], blocked: [41, 40, 41], en: 'Lava', ru: 'Лава' },
	{ id: 'water', letter: 'w', color: [8, 81, 148], blocked: [8, 81, 148], en: 'Water', ru: 'Вода' },
	{ id: 'rock', letter: 'k', color: [0, 0, 0], blocked: [0, 0, 0], en: 'Rock', ru: 'Скалы' },
	// Horn of the Abyss terrains, for pictures exported from maps that use them; the map builder knows only core terrains
	{ id: 'highlands', letter: 'h', color: [84, 121, 26], blocked: [41, 106, 20], en: 'Highlands (HotA)', ru: 'Нагорье (HotA)', hota: true },
	{ id: 'wasteland', letter: 'a', color: [180, 105, 39], blocked: [164, 86, 29], en: 'Wasteland (HotA)', ru: 'Пустошь (HotA)', hota: true },
];
const WATER = TERRAINS.findIndex((t) => t.id === 'water');
/// Typical colours of maps, satellite pictures and paintings, with the terrain and obstacles suggested for them
const SUGGESTIONS = [
	[[30, 80, 150], 'water'], [[60, 120, 190], 'water'], [[18, 45, 95], 'water'], [[5, 39, 71], 'water'], [[26, 62, 95], 'water'],
	[[10, 25, 50], 'water'], [[140, 195, 230], 'water'],
	[[95, 145, 60], 'grass'], [[60, 110, 45], 'grass'], [[130, 170, 90], 'grass'], [[100, 100, 55], 'grass'], [[88, 86, 48], 'grass'],
	[[28, 30, 15], 'grass', true], [[57, 62, 26], 'grass', true], [[35, 60, 30], 'grass', true], [[20, 45, 20], 'grass', true],
	[[118, 88, 52], 'dirt'], [[95, 70, 45], 'dirt'],
	[[222, 200, 150], 'sand'], [[200, 170, 110], 'sand'], [[235, 220, 180], 'sand'],
	[[150, 132, 95], 'rough'], [[125, 112, 82], 'rough'], [[170, 150, 120], 'rough'], [[86, 109, 132], 'rough', true], [[110, 105, 100], 'rough', true],
	[[242, 246, 250], 'snow'], [[205, 215, 225], 'snow'], [[165, 175, 188], 'snow'],
	[[70, 95, 72], 'swamp'], [[85, 110, 90], 'swamp'],
	[[160, 45, 25], 'lava'], [[90, 30, 20], 'lava'],
];
const GRASS = TERRAINS.findIndex((t) => t.id === 'grass');
const ROCK = TERRAINS.findIndex((t) => t.id === 'rock');
const MAX_SIDE = 512;
const API_URL = '../api/map';
const DRAFT_KEY = 'vcmi-terrain-painter-draft';

const TEXT = {
	en: {
		title: 'Terrain Painter', mapSection: '1. Map', size: 'Size', custom: 'Custom', resize: 'Resize', toroidal: 'Toroidal: edges wrap around',
		newMap: 'New map', filledWith: 'filled with', imageSection: '2. Picture → terrain',
		dropHint: 'Drop a picture here, paste it or click to choose. It stays on your device.', fit: 'Fit', fitCover: 'Crop to fill',
		fitContain: 'Fit with margins', fitStretch: 'Stretch', matchAspect: 'Match picture shape', colors: 'Colour groups', share: 'Share',
		terrain: 'Terrain', obstaclesShort: 'Obstacles', smoothing: 'Smoothing', minRegion: 'Remove spots smaller than', apply: 'Use as map',
		discard: 'Discard', underlay: 'Show picture under the map', paintSection: '3. Paint', toolBrush: 'Brush (B)', toolEraser: 'Eraser (E)',
		toolFill: 'Fill (F); Shift + click fills the whole map', toolPicker: 'Pick terrain (I)', toolPan: 'Move view (H)', undo: 'Undo (Ctrl+Z)', redo: 'Redo (Ctrl+Shift+Z)',
		layerTerrain: 'Terrain', layerObstacles: 'Obstacles', brushSize: 'Brush', torusPreview: 'Show wrapped copies around the map',
		grid: 'Grid when zoomed in', paintHint: 'Pen or mouse paints. Two-finger scrolling moves the view, pinch or Cmd/Ctrl + scroll zooms. Space + drag, the middle button or the hand tool also move the view; + and − zoom.',
		buildSection: '4. Get the map', mapName: 'Name', mapDescription: 'Description', build: 'Build .vmap',
		showChanged: 'Highlight cells the engine changed', acceptChanges: 'Copy engine changes into the drawing',
		buildHint: 'Only the terrain grid is sent to the server. Towns, heroes and objects are placed afterwards in the VCMI map editor.',
		exportPng: 'Save PNG', importPng: 'Open PNG', exportPalette: 'Palette .gpl', fitView: 'Fit',
		previewActive: 'Preview of the picture. Press “Use as map” to start painting on it.', converting: 'Converting…',
		building: 'Building the map…', built: 'Map built: {obstacles} obstacles; the engine changed {changed} cells to make transitions valid.',
		builtClean: 'Map built: {obstacles} obstacles; every cell kept its terrain.', buildFailed: 'Building failed: {error}',
		pngTooLarge: 'The PNG is larger than {max} × {max}; use “Picture → terrain” for photos.', imageFailed: 'Cannot read this picture.',
		defaultName: 'Painted map', obstacles: 'obstacles', cell: 'cell', withObstacles: 'with obstacles', nothingToAccept: 'No changed cells.',
		savedDraft: 'Draft restored from this browser.',
		toolBrushLabel: 'Brush', toolEraserLabel: 'Eraser', toolFillLabel: 'Fill', toolPickerLabel: 'Picker', toolPanLabel: 'Move',
		undoLabel: 'Undo', redoLabel: 'Redo', fillArea: 'Connected area', fillAll: 'Whole map',
		hotaBuild: 'HotA terrains cannot be built on the server. Save the PNG and import it in the VCMI map editor with HotA enabled.',
		showObjects: 'Show objects from the map editor',
		roundTripHint: 'To repaint an existing map, use File → Export terrain to PNG in the VCMI map editor, open both PNG files here, then import the saved PNG back with File → Import terrain from PNG.',
		objectsSizeMismatch: 'The objects picture is {w} × {h}, but the map is {mw} × {mh}.',
	},
	ru: {
		title: 'Рисование ландшафта', mapSection: '1. Карта', size: 'Размер', custom: 'Свой', resize: 'Изменить', toroidal: 'Тор: края карты склеены',
		newMap: 'Новая карта', filledWith: 'заполнить', imageSection: '2. Картинка → ландшафт',
		dropHint: 'Перетащите картинку сюда, вставьте её или нажмите, чтобы выбрать. Она не покидает ваше устройство.', fit: 'Вписать',
		fitCover: 'Обрезать по карте', fitContain: 'Вписать с полями', fitStretch: 'Растянуть', matchAspect: 'Пропорции картинки',
		colors: 'Групп цветов', share: 'Доля', terrain: 'Земля', obstaclesShort: 'Препятствия', smoothing: 'Сглаживание',
		minRegion: 'Убрать пятна меньше', apply: 'Взять как карту', discard: 'Отменить', underlay: 'Показать картинку под картой',
		paintSection: '3. Рисование', toolBrush: 'Кисть (B)', toolEraser: 'Ластик (E)', toolFill: 'Заливка (F); Shift + клик — по всей карте', toolPicker: 'Пипетка (I)',
		toolPan: 'Двигать вид (H)', undo: 'Отменить (Ctrl+Z)', redo: 'Повторить (Ctrl+Shift+Z)', layerTerrain: 'Земля',
		layerObstacles: 'Препятствия', brushSize: 'Кисть', torusPreview: 'Показывать склеенные копии вокруг карты',
		grid: 'Сетка при увеличении', paintHint: 'Рисует стилус или мышь. Прокрутка двумя пальцами двигает вид, щипок или Cmd/Ctrl + прокрутка меняют масштаб. Ещё вид двигают пробел + перетаскивание, средняя кнопка и инструмент «рука»; + и − меняют масштаб.',
		buildSection: '4. Получить карту', mapName: 'Название', mapDescription: 'Описание', build: 'Собрать .vmap',
		showChanged: 'Подсветить клетки, которые изменил движок', acceptChanges: 'Перенести изменения движка в рисунок',
		buildHint: 'На сервер уходит только сетка земель. Города, героев и объекты потом расставляют в редакторе карт VCMI.',
		exportPng: 'Сохранить PNG', importPng: 'Открыть PNG', exportPalette: 'Палитра .gpl', fitView: 'Вся карта',
		previewActive: 'Это превью картинки. Нажмите «Взять как карту», чтобы рисовать поверх неё.', converting: 'Преобразую…',
		building: 'Собираю карту…', built: 'Карта собрана: препятствий {obstacles}; движок изменил {changed} клеток, чтобы переходы были допустимы.',
		builtClean: 'Карта собрана: препятствий {obstacles}; все клетки сохранили свою землю.', buildFailed: 'Не удалось собрать карту: {error}',
		pngTooLarge: 'PNG больше {max} × {max}; для фотографий используйте «Картинка → ландшафт».', imageFailed: 'Не удалось прочитать картинку.',
		defaultName: 'Нарисованная карта', obstacles: 'препятствия', cell: 'клетка', withObstacles: 'с препятствиями', nothingToAccept: 'Изменённых клеток нет.',
		savedDraft: 'Черновик восстановлен из этого браузера.',
		toolBrushLabel: 'Кисть', toolEraserLabel: 'Ластик', toolFillLabel: 'Заливка', toolPickerLabel: 'Пипетка', toolPanLabel: 'Рука',
		undoLabel: 'Отменить', redoLabel: 'Повторить', fillArea: 'Связная область', fillAll: 'Вся карта',
		hotaBuild: 'Земли HotA сервер собрать не может. Сохраните PNG и импортируйте его в редакторе карт VCMI с включённым HotA.',
		showObjects: 'Показывать объекты из редактора карт',
		roundTripHint: 'Чтобы перерисовать готовую карту, в редакторе карт VCMI выберите «Файл → Экспорт ландшафта в PNG», откройте здесь оба PNG, а сохранённый PNG загрузите обратно через «Файл → Импорт ландшафта из PNG».',
		objectsSizeMismatch: 'Картинка объектов {w} × {h}, а карта {mw} × {mh}.',
	},
};

const $ = (id) => document.getElementById(id);
const view = $('view');
const ctx = view.getContext('2d');

const state = {
	width: 144,
	height: 144,
	toroidal: true,
	terrain: new Uint8Array(144 * 144).fill(WATER),
	obstacles: new Uint8Array(144 * 144),
	current: GRASS,
	tool: 'brush',
	layer: 'terrain',
	brush: 3,
	zoom: 4,
	offsetX: 0,
	offsetY: 0,
	changed: new Set(),
	changedTerrain: new Map(),
	preview: null,
	lang: 'en',
};
let undoStack = [];
let redoStack = [];

// ---------- text ----------

function t(key, values = {})
{
	const text = TEXT[state.lang][key] ?? TEXT.en[key] ?? key;
	return text.replace(/\{(\w+)\}/g, (_, name) => String(values[name] ?? ''));
}

function terrainName(index)
{
	return TERRAINS[index][state.lang] ?? TERRAINS[index].en;
}

function applyLanguage()
{
	document.documentElement.lang = state.lang;
	for (const element of document.querySelectorAll('[data-i18n]'))
		element.textContent = t(element.dataset.i18n);
	for (const element of document.querySelectorAll('[data-i18n-title]'))
		element.title = t(element.dataset.i18nTitle);
	document.title = t('title');
	buildPalette();
	buildFillSelect();
	if (converter.clusters)
		renderClusterRows();
}

function setStatus(message, error = false)
{
	const status = $('status');
	status.textContent = message;
	status.classList.toggle('error', error);
}

// ---------- storage ----------

function toBase64(bytes)
{
	let binary = '';
	for (let i = 0; i < bytes.length; i += 0x8000)
		binary += String.fromCharCode.apply(null, bytes.subarray(i, i + 0x8000));
	return btoa(binary);
}

function fromBase64(text)
{
	const binary = atob(text);
	const bytes = new Uint8Array(binary.length);
	for (let i = 0; i < binary.length; i++)
		bytes[i] = binary.charCodeAt(i);
	return bytes;
}

let draftTimer = 0;
function scheduleDraftSave()
{
	clearTimeout(draftTimer);
	draftTimer = setTimeout(() =>
	{
		try
		{
			localStorage.setItem(DRAFT_KEY, JSON.stringify({
				width: state.width, height: state.height, toroidal: state.toroidal,
				terrain: toBase64(state.terrain), obstacles: toBase64(state.obstacles),
				name: $('mapName').value, description: $('mapDescription').value,
			}));
		}
		catch (error)
		{
			console.warn('Draft not saved', error);
		}
	}, 800);
}

function loadDraft()
{
	try
	{
		const draft = JSON.parse(localStorage.getItem(DRAFT_KEY) ?? 'null');
		if (!draft)
			return false;
		const terrain = fromBase64(draft.terrain);
		const obstacles = fromBase64(draft.obstacles);
		if (terrain.length !== draft.width * draft.height || obstacles.length !== terrain.length || terrain.some((v) => v >= TERRAINS.length))
			return false;
		Object.assign(state, { width: draft.width, height: draft.height, toroidal: !!draft.toroidal, terrain, obstacles });
		$('mapName').value = draft.name ?? '';
		$('mapDescription').value = draft.description ?? '';
		return true;
	}
	catch (error)
	{
		console.warn('Draft not restored', error);
		return false;
	}
}

// ---------- history ----------

function snapshot()
{
	return { width: state.width, height: state.height, terrain: state.terrain.slice(), obstacles: state.obstacles.slice() };
}

function restore(shot)
{
	state.width = shot.width;
	state.height = shot.height;
	state.terrain = shot.terrain.slice();
	state.obstacles = shot.obstacles.slice();
	syncSizeInputs();
}

/// Records a change of the whole map (conversion, resize, import) as one undo step
function commitWhole(change)
{
	const before = snapshot();
	change();
	undoStack.push({ before, after: snapshot() });
	if (undoStack.length > 60)
		undoStack.shift();
	redoStack = [];
	clearChanged();
	afterEdit(true);
}

function undo()
{
	const entry = undoStack.pop();
	if (!entry)
		return;
	if (entry.before)
		restore(entry.before);
	else
		for (let i = 0; i < entry.cells.length; i++)
		{
			state.terrain[entry.cells[i]] = entry.oldTerrain[i];
			state.obstacles[entry.cells[i]] = entry.oldObstacles[i];
		}
	redoStack.push(entry);
	afterEdit(!!entry.before);
}

function redo()
{
	const entry = redoStack.pop();
	if (!entry)
		return;
	if (entry.after)
		restore(entry.after);
	else
		for (let i = 0; i < entry.cells.length; i++)
		{
			state.terrain[entry.cells[i]] = entry.newTerrain[i];
			state.obstacles[entry.cells[i]] = entry.newObstacles[i];
		}
	undoStack.push(entry);
	afterEdit(!!entry.after);
}

function afterEdit(resized = false)
{
	$('undo').disabled = undoStack.length === 0;
	$('redo').disabled = redoStack.length === 0;
	cellsDirty = true;
	if (resized)
		fitView();
	scheduleDraftSave();
	requestRender();
}

// ---------- editing ----------

let stroke = null;

function beginStroke()
{
	stroke = new Map();
}

function setCell(index, terrain, obstacle)
{
	if (state.terrain[index] === terrain && state.obstacles[index] === obstacle)
		return;
	if (stroke && !stroke.has(index))
		stroke.set(index, [state.terrain[index], state.obstacles[index]]);
	state.terrain[index] = terrain;
	state.obstacles[index] = obstacle;
	state.changed.delete(index);
	state.changedTerrain.delete(index);
	cellsDirty = true;
}

function endStroke()
{
	if (!stroke)
		return;
	if (stroke.size > 0)
	{
		const cells = Int32Array.from(stroke.keys());
		const entry = {
			cells,
			oldTerrain: Uint8Array.from(cells, (c) => stroke.get(c)[0]),
			oldObstacles: Uint8Array.from(cells, (c) => stroke.get(c)[1]),
			newTerrain: Uint8Array.from(cells, (c) => state.terrain[c]),
			newObstacles: Uint8Array.from(cells, (c) => state.obstacles[c]),
		};
		undoStack.push(entry);
		if (undoStack.length > 200)
			undoStack.shift();
		redoStack = [];
	}
	stroke = null;
	afterEdit();
}

function cancelStroke()
{
	if (!stroke)
		return;
	for (const [index, [terrain, obstacle]] of stroke)
	{
		state.terrain[index] = terrain;
		state.obstacles[index] = obstacle;
	}
	stroke = null;
	afterEdit();
}

/// Returns the cell index, wrapping on a toroidal map, or -1 outside the map
function cellIndex(x, y)
{
	if (state.toroidal)
	{
		x = ((x % state.width) + state.width) % state.width;
		y = ((y % state.height) + state.height) % state.height;
	}
	else if (x < 0 || y < 0 || x >= state.width || y >= state.height)
		return -1;
	return y * state.width + x;
}

function paintCell(index, erase)
{
	if (index < 0)
		return;
	if (state.layer === 'terrain')
	{
		const terrain = erase ? WATER : state.current;
		setCell(index, terrain, state.obstacles[index]);
	}
	else
		setCell(index, state.terrain[index], erase ? 0 : 1);
}

function stamp(cx, cy, erase)
{
	const radius = (state.brush - 1) / 2;
	const reach = Math.ceil(radius);
	const limit = radius * radius + radius * 0.8 + 0.01;
	for (let dy = -reach; dy <= reach; dy++)
		for (let dx = -reach; dx <= reach; dx++)
			if (dx * dx + dy * dy <= limit)
				paintCell(cellIndex(cx + dx, cy + dy), erase);
}

function strokeLine(x0, y0, x1, y1, erase)
{
	const dx = Math.abs(x1 - x0);
	const dy = Math.abs(y1 - y0);
	const sx = x0 < x1 ? 1 : -1;
	const sy = y0 < y1 ? 1 : -1;
	let error = dx - dy;
	for (;;)
	{
		stamp(x0, y0, erase);
		if (x0 === x1 && y0 === y1)
			break;
		const e2 = 2 * error;
		if (e2 > -dy)
		{
			error -= dy;
			x0 += sx;
		}
		if (e2 < dx)
		{
			error += dx;
			y0 += sy;
		}
	}
}

/// Fills the connected area of the clicked terrain, or every cell of that terrain when wholeMap is set
function floodFill(cx, cy, wholeMap)
{
	const start = cellIndex(cx, cy);
	if (start < 0)
		return;
	const target = state.terrain[start];
	const fillCell = (index) =>
	{
		if (state.layer === 'terrain')
			setCell(index, state.current, state.obstacles[index]);
		else
			setCell(index, state.terrain[index], state.tool === 'eraser' ? 0 : 1);
	};
	beginStroke();
	if (wholeMap)
	{
		for (let index = 0; index < state.terrain.length; index++)
			if (state.terrain[index] === target)
				fillCell(index);
		endStroke();
		return;
	}
	const { width, height } = state;
	const seen = new Uint8Array(width * height);
	const queue = [start];
	seen[start] = 1;
	while (queue.length)
	{
		const index = queue.pop();
		fillCell(index);
		const x = index % width;
		const y = (index - x) / width;
		for (const [nx, ny] of [[x + 1, y], [x - 1, y], [x, y + 1], [x, y - 1]])
		{
			const next = cellIndex(nx, ny);
			if (next >= 0 && !seen[next] && state.terrain[next] === target)
			{
				seen[next] = 1;
				queue.push(next);
			}
		}
	}
	endStroke();
}

function resample(width, height)
{
	const terrain = new Uint8Array(width * height);
	const obstacles = new Uint8Array(width * height);
	for (let y = 0; y < height; y++)
	{
		const sy = Math.min(state.height - 1, Math.floor(((y + 0.5) * state.height) / height));
		for (let x = 0; x < width; x++)
		{
			const sx = Math.min(state.width - 1, Math.floor(((x + 0.5) * state.width) / width));
			terrain[y * width + x] = state.terrain[sy * state.width + sx];
			obstacles[y * width + x] = state.obstacles[sy * state.width + sx];
		}
	}
	state.width = width;
	state.height = height;
	state.terrain = terrain;
	state.obstacles = obstacles;
}

function clearChanged()
{
	state.changed.clear();
	state.changedTerrain.clear();
	$('buildResult').hidden = true;
}

// ---------- rendering ----------

const cells = document.createElement('canvas');
const cellsContext = cells.getContext('2d');
let cellsDirty = true;
let renderQueued = false;
let hover = null;
let underlay = null;
let objectsOverlay = null;

function displayColor(terrain, obstacle)
{
	const base = TERRAINS[terrain].color;
	if (!obstacle || TERRAINS[terrain].id === 'water' || terrain === ROCK)
		return base;
	// darkened and tinted so that obstacle areas stand out on every terrain
	return [base[0] * 0.45 + 60 * 0.55, base[1] * 0.45 + 30 * 0.55, base[2] * 0.45 + 10 * 0.55];
}

function refreshCells()
{
	const source = state.preview ?? state;
	if (cells.width !== source.width || cells.height !== source.height)
	{
		cells.width = source.width;
		cells.height = source.height;
	}
	const image = cellsContext.createImageData(source.width, source.height);
	for (let i = 0; i < source.terrain.length; i++)
	{
		const color = displayColor(source.terrain[i], source.obstacles[i]);
		image.data[i * 4] = color[0];
		image.data[i * 4 + 1] = color[1];
		image.data[i * 4 + 2] = color[2];
		image.data[i * 4 + 3] = 255;
	}
	cellsContext.putImageData(image, 0, 0);
	cellsDirty = false;
}

function requestRender()
{
	if (renderQueued)
		return;
	renderQueued = true;
	requestAnimationFrame(render);
}

function render()
{
	renderQueued = false;
	const ratio = window.devicePixelRatio || 1;
	const cssWidth = view.clientWidth;
	const cssHeight = view.clientHeight;
	if (view.width !== Math.round(cssWidth * ratio) || view.height !== Math.round(cssHeight * ratio))
	{
		view.width = Math.round(cssWidth * ratio);
		view.height = Math.round(cssHeight * ratio);
	}
	if (cellsDirty)
		refreshCells();

	const { zoom, offsetX, offsetY } = state;
	const source = state.preview ?? state;
	const mapWidth = source.width * zoom;
	const mapHeight = source.height * zoom;
	ctx.setTransform(ratio, 0, 0, ratio, 0, 0);
	ctx.fillStyle = '#0a100f';
	ctx.fillRect(0, 0, cssWidth, cssHeight);
	ctx.imageSmoothingEnabled = false;

	const wrapped = state.toroidal && $('torusPreview').checked;
	for (let dy = -1; dy <= 1; dy++)
	{
		for (let dx = -1; dx <= 1; dx++)
		{
			if ((dx || dy) && !wrapped)
				continue;
			ctx.globalAlpha = dx || dy ? 0.45 : 1;
			ctx.drawImage(cells, offsetX + dx * mapWidth, offsetY + dy * mapHeight, mapWidth, mapHeight);
		}
	}
	ctx.globalAlpha = 1;

	if (objectsOverlay && !state.preview && $('showObjects').checked && objectsOverlay.width === state.width && objectsOverlay.height === state.height)
		for (let dy = -1; dy <= 1; dy++)
			for (let dx = -1; dx <= 1; dx++)
				if (!(dx || dy) || wrapped)
					ctx.drawImage(objectsOverlay, offsetX + dx * mapWidth, offsetY + dy * mapHeight, mapWidth, mapHeight);

	if (underlay && $('showUnderlay').checked)
	{
		ctx.globalAlpha = Number($('underlayOpacity').value) / 100;
		ctx.imageSmoothingEnabled = true;
		const rect = fitRect(underlay.width, underlay.height, mapWidth, mapHeight, $('fitMode').value);
		ctx.save();
		ctx.beginPath();
		ctx.rect(offsetX, offsetY, mapWidth, mapHeight);
		ctx.clip();
		ctx.drawImage(underlay, offsetX + rect.x, offsetY + rect.y, rect.width, rect.height);
		ctx.restore();
		ctx.globalAlpha = 1;
		ctx.imageSmoothingEnabled = false;
	}

	if ($('showGrid').checked && zoom >= 8)
	{
		ctx.strokeStyle = 'rgba(0,0,0,0.25)';
		ctx.lineWidth = 1;
		ctx.beginPath();
		const x0 = Math.max(0, Math.floor(-offsetX / zoom));
		const x1 = Math.min(source.width, Math.ceil((cssWidth - offsetX) / zoom));
		const y0 = Math.max(0, Math.floor(-offsetY / zoom));
		const y1 = Math.min(source.height, Math.ceil((cssHeight - offsetY) / zoom));
		for (let x = x0; x <= x1; x++)
		{
			ctx.moveTo(Math.round(offsetX + x * zoom) + 0.5, offsetY + y0 * zoom);
			ctx.lineTo(Math.round(offsetX + x * zoom) + 0.5, offsetY + y1 * zoom);
		}
		for (let y = y0; y <= y1; y++)
		{
			ctx.moveTo(offsetX + x0 * zoom, Math.round(offsetY + y * zoom) + 0.5);
			ctx.lineTo(offsetX + x1 * zoom, Math.round(offsetY + y * zoom) + 0.5);
		}
		ctx.stroke();
	}

	if (!state.preview && state.changed.size && $('showChanged').checked)
	{
		ctx.strokeStyle = '#ff4d3d';
		ctx.fillStyle = 'rgba(255,77,61,0.35)';
		ctx.lineWidth = Math.max(1, Math.min(2, zoom / 6));
		for (const index of state.changed)
		{
			const x = offsetX + (index % state.width) * zoom;
			const y = offsetY + Math.floor(index / state.width) * zoom;
			if (zoom >= 5)
				ctx.strokeRect(x + 0.5, y + 0.5, zoom - 1, zoom - 1);
			else
				ctx.fillRect(x, y, zoom, zoom);
		}
	}

	ctx.strokeStyle = '#e4bd75';
	ctx.lineWidth = 1;
	ctx.strokeRect(offsetX - 0.5, offsetY - 0.5, mapWidth + 1, mapHeight + 1);

	if (hover && !state.preview && (state.tool === 'brush' || state.tool === 'eraser'))
	{
		ctx.strokeStyle = 'rgba(255,255,255,0.8)';
		ctx.beginPath();
		ctx.arc(offsetX + (hover.x + 0.5) * zoom, offsetY + (hover.y + 0.5) * zoom, Math.max(2, (state.brush / 2) * zoom), 0, Math.PI * 2);
		ctx.stroke();
	}
}

function fitView()
{
	const source = state.preview ?? state;
	const margin = state.toroidal && $('torusPreview').checked ? 60 : 24;
	const zoom = Math.min((view.clientWidth - margin * 2) / source.width, (view.clientHeight - margin * 2) / source.height);
	state.zoom = Math.max(0.5, zoom);
	state.offsetX = (view.clientWidth - source.width * state.zoom) / 2;
	state.offsetY = (view.clientHeight - source.height * state.zoom) / 2;
	requestRender();
}

function zoomAt(factor, x, y)
{
	const zoom = Math.min(64, Math.max(0.5, state.zoom * factor));
	state.offsetX = x - ((x - state.offsetX) * zoom) / state.zoom;
	state.offsetY = y - ((y - state.offsetY) * zoom) / state.zoom;
	state.zoom = zoom;
	requestRender();
}

function screenToCell(x, y)
{
	return { x: Math.floor((x - state.offsetX) / state.zoom), y: Math.floor((y - state.offsetY) / state.zoom) };
}

// ---------- pointer input ----------

const pointers = new Map();
let penSeen = false;
let spaceDown = false;
let gesture = null;
let drag = null;

function localPoint(event)
{
	const rect = view.getBoundingClientRect();
	return { x: event.clientX - rect.left, y: event.clientY - rect.top };
}

function startGesture()
{
	const points = [...pointers.values()];
	if (points.length >= 2)
	{
		const [a, b] = points;
		gesture = { distance: Math.hypot(a.x - b.x, a.y - b.y), mid: { x: (a.x + b.x) / 2, y: (a.y + b.y) / 2 } };
	}
	else
		gesture = { distance: 0, mid: { ...points[0] } };
}

view.addEventListener('pointerdown', (event) =>
{
	view.setPointerCapture(event.pointerId);
	const point = localPoint(event);
	pointers.set(event.pointerId, point);
	if (event.pointerType === 'pen')
		penSeen = true;

	const touchGesture = event.pointerType === 'touch' && (penSeen || pointers.size > 1);
	if (touchGesture || pointers.size > 1)
	{
		cancelStroke();
		drag = null;
		startGesture();
		return;
	}
	if (event.button === 1 || spaceDown || state.tool === 'pan')
	{
		drag = { kind: 'pan', x: point.x, y: point.y };
		return;
	}
	if (state.preview)
	{
		setStatus(t('previewActive'));
		return;
	}
	const cell = screenToCell(point.x, point.y);
	const erase = state.tool === 'eraser' || event.button === 2;
	if (state.tool === 'picker' || event.altKey)
	{
		const index = cellIndex(cell.x, cell.y);
		if (index >= 0)
			selectTerrain(state.terrain[index]);
		return;
	}
	if (state.tool === 'fill')
	{
		floodFill(cell.x, cell.y, event.shiftKey || document.querySelector('input[name=fillMode]:checked').value === 'all');
		return;
	}
	beginStroke();
	drag = { kind: 'paint', cell, erase };
	stamp(cell.x, cell.y, erase);
	requestRender();
});

view.addEventListener('pointermove', (event) =>
{
	const point = localPoint(event);
	if (pointers.has(event.pointerId))
		pointers.set(event.pointerId, point);
	const cell = screenToCell(point.x, point.y);
	hover = cell;
	updateCursorInfo(cell);

	if (gesture)
	{
		const points = [...pointers.values()];
		if (points.length >= 2 && gesture.distance > 0)
		{
			const [a, b] = points;
			const distance = Math.hypot(a.x - b.x, a.y - b.y);
			const mid = { x: (a.x + b.x) / 2, y: (a.y + b.y) / 2 };
			state.offsetX += mid.x - gesture.mid.x;
			state.offsetY += mid.y - gesture.mid.y;
			zoomAt(distance / gesture.distance, mid.x, mid.y);
			gesture = { distance, mid };
		}
		else if (points.length === 1)
		{
			state.offsetX += points[0].x - gesture.mid.x;
			state.offsetY += points[0].y - gesture.mid.y;
			gesture.mid = { ...points[0] };
		}
		requestRender();
		return;
	}
	if (drag?.kind === 'pan')
	{
		state.offsetX += point.x - drag.x;
		state.offsetY += point.y - drag.y;
		drag.x = point.x;
		drag.y = point.y;
	}
	else if (drag?.kind === 'paint')
	{
		const events = event.getCoalescedEvents ? event.getCoalescedEvents() : [event];
		for (const sample of events)
		{
			const p = localPoint(sample);
			const next = screenToCell(p.x, p.y);
			strokeLine(drag.cell.x, drag.cell.y, next.x, next.y, drag.erase);
			drag.cell = next;
		}
	}
	requestRender();
});

function releasePointer(event)
{
	pointers.delete(event.pointerId);
	if (gesture)
	{
		if (pointers.size === 0)
			gesture = null;
		else
			startGesture();
		return;
	}
	if (drag?.kind === 'paint')
		endStroke();
	drag = null;
}

view.addEventListener('pointerup', releasePointer);
view.addEventListener('pointercancel', (event) =>
{
	if (drag?.kind === 'paint')
		cancelStroke();
	drag = null;
	releasePointer(event);
});
view.addEventListener('pointerleave', () =>
{
	hover = null;
	$('cursorInfo').textContent = '';
	requestRender();
});
view.addEventListener('contextmenu', (event) => event.preventDefault());
view.addEventListener('wheel', (event) =>
{
	event.preventDefault();
	const point = localPoint(event);
	// Trackpad scrolling moves the view; pinch (reported with ctrlKey), Cmd/Ctrl + wheel and notched mouse wheels zoom
	const notchedWheel = event.deltaMode !== 0 || (event.deltaX === 0 && event.wheelDeltaY !== undefined && event.wheelDeltaY !== 0 && event.wheelDeltaY % 120 === 0);
	if (event.ctrlKey || event.metaKey || notchedWheel)
	{
		const step = event.deltaMode === 1 ? event.deltaY * 33 : event.deltaY;
		zoomAt(Math.exp(-step * (event.ctrlKey && !notchedWheel ? 0.02 : 0.0015)), point.x, point.y);
	}
	else
	{
		state.offsetX -= event.shiftKey && !event.deltaX ? event.deltaY : event.deltaX;
		state.offsetY -= event.shiftKey && !event.deltaX ? 0 : event.deltaY;
		requestRender();
	}
}, { passive: false });

function updateCursorInfo(cell)
{
	const source = state.preview ?? state;
	let { x, y } = cell;
	if (state.toroidal)
	{
		x = ((x % source.width) + source.width) % source.width;
		y = ((y % source.height) + source.height) % source.height;
	}
	if (x < 0 || y < 0 || x >= source.width || y >= source.height)
	{
		$('cursorInfo').textContent = '';
		return;
	}
	const index = y * source.width + x;
	let text = `${x}, ${y} · ${terrainName(source.terrain[index])}`;
	if (source.obstacles[index])
		text += ` · ${t('obstacles')}`;
	$('cursorInfo').textContent = text;
}

// ---------- keyboard ----------

window.addEventListener('keydown', (event) =>
{
	if (event.target.closest('input[type=text],textarea,input[type=number]'))
		return;
	const key = event.key.toLowerCase();
	const command = event.metaKey || event.ctrlKey;
	if (command && key === 'z')
	{
		event.preventDefault();
		event.shiftKey ? redo() : undo();
		return;
	}
	if (command && key === 'y')
	{
		event.preventDefault();
		redo();
		return;
	}
	if (command)
		return;
	if (event.code === 'Space')
	{
		spaceDown = true;
		event.preventDefault();
		return;
	}
	if (key === '+' || key === '=')
		return zoomAt(1.25, view.clientWidth / 2, view.clientHeight / 2);
	if (key === '-')
		return zoomAt(0.8, view.clientWidth / 2, view.clientHeight / 2);
	const tools = { b: 'brush', e: 'eraser', f: 'fill', i: 'picker', h: 'pan' };
	if (tools[key])
		selectTool(tools[key]);
	else if (key === '[')
		setBrush(state.brush - 2);
	else if (key === ']')
		setBrush(state.brush + 2);
	else if (key === 'o')
		selectLayer(state.layer === 'terrain' ? 'obstacles' : 'terrain');
	else if (/^[0-9]$/.test(key))
	{
		const index = key === '0' ? 9 : Number(key) - 1;
		if (index < TERRAINS.length)
			selectTerrain(index);
	}
});
window.addEventListener('keyup', (event) =>
{
	if (event.code === 'Space')
		spaceDown = false;
});

// ---------- controls ----------

function selectTool(tool)
{
	state.tool = tool;
	for (const button of document.querySelectorAll('[data-tool]'))
		button.classList.toggle('active', button.dataset.tool === tool);
	view.style.cursor = tool === 'pan' ? 'grab' : 'crosshair';
	$('fillModeRow').hidden = tool !== 'fill';
	requestRender();
}

function selectLayer(layer)
{
	state.layer = layer;
	for (const input of document.querySelectorAll('input[name=layer]'))
		input.checked = input.value === layer;
}

function setBrush(size)
{
	state.brush = Math.max(1, Math.min(31, size));
	$('brushSize').value = state.brush;
	$('brushSizeOut').textContent = state.brush;
	requestRender();
}

function selectTerrain(index)
{
	state.current = index;
	selectLayer('terrain');
	if (state.tool === 'picker' || state.tool === 'eraser')
		selectTool('brush');
	for (const button of $('palette').children)
		button.classList.toggle('active', Number(button.dataset.terrain) === index);
}

function swatch(color)
{
	const span = document.createElement('span');
	span.className = 'swatch';
	span.style.background = `rgb(${color.join(',')})`;
	return span;
}

function buildPalette()
{
	const palette = $('palette');
	palette.replaceChildren();
	TERRAINS.forEach((terrain, index) =>
	{
		const button = document.createElement('button');
		button.type = 'button';
		button.dataset.terrain = index;
		button.append(swatch(terrain.color), terrainName(index));
		const key = document.createElement('kbd');
		key.textContent = index < 9 ? String(index + 1) : index === 9 ? '0' : '';
		button.append(key);
		button.classList.toggle('active', index === state.current);
		button.addEventListener('click', () => selectTerrain(index));
		palette.append(button);
	});
}

function buildFillSelect()
{
	const select = $('fillTerrain');
	const value = select.value || String(WATER);
	select.replaceChildren(...TERRAINS.map((terrain, index) => new Option(terrainName(index), index)));
	select.value = value;
}

function syncSizeInputs()
{
	$('mapWidth').value = state.width;
	$('mapHeight').value = state.height;
	const preset = state.width === state.height && $('sizePreset').querySelector(`option[value="${state.width}"]`);
	$('sizePreset').value = preset ? String(state.width) : 'custom';
}

function requestedSize()
{
	const clamp = (value) => Math.max(8, Math.min(MAX_SIDE, Math.round(Number(value) || 8)));
	return { width: clamp($('mapWidth').value), height: clamp($('mapHeight').value) };
}

$('sizePreset').addEventListener('change', () =>
{
	if ($('sizePreset').value === 'custom')
		return;
	$('mapWidth').value = $('sizePreset').value;
	$('mapHeight').value = $('sizePreset').value;
	if (converter.image)
		scheduleConversion();
});
for (const id of ['mapWidth', 'mapHeight'])
	$(id).addEventListener('change', () =>
	{
		$('sizePreset').value = 'custom';
		if (converter.image)
			scheduleConversion();
	});
$('resizeMap').addEventListener('click', () =>
{
	const { width, height } = requestedSize();
	commitWhole(() => resample(width, height));
	syncSizeInputs();
});
$('newMap').addEventListener('click', () =>
{
	const { width, height } = requestedSize();
	const fill = Number($('fillTerrain').value);
	commitWhole(() =>
	{
		state.width = width;
		state.height = height;
		state.terrain = new Uint8Array(width * height).fill(fill);
		state.obstacles = new Uint8Array(width * height);
	});
	syncSizeInputs();
});
$('toroidal').addEventListener('change', () =>
{
	state.toroidal = $('toroidal').checked;
	scheduleDraftSave();
	fitView();
});
for (const button of document.querySelectorAll('[data-tool]'))
	button.addEventListener('click', () => selectTool(button.dataset.tool));
for (const input of document.querySelectorAll('input[name=layer]'))
	input.addEventListener('change', () => selectLayer(input.value));
$('brushSize').addEventListener('input', () => setBrush(Number($('brushSize').value)));
$('undo').addEventListener('click', undo);
$('redo').addEventListener('click', redo);
for (const id of ['torusPreview', 'showGrid', 'showUnderlay', 'underlayOpacity', 'showChanged', 'showObjects'])
	$(id).addEventListener('input', requestRender);
$('torusPreview').addEventListener('change', fitView);
$('zoomIn').addEventListener('click', () => zoomAt(1.25, view.clientWidth / 2, view.clientHeight / 2));
$('zoomOut').addEventListener('click', () => zoomAt(0.8, view.clientWidth / 2, view.clientHeight / 2));
$('zoomFit').addEventListener('click', fitView);
for (const id of ['mapName', 'mapDescription'])
	$(id).addEventListener('input', scheduleDraftSave);
$('lang').addEventListener('change', () =>
{
	state.lang = $('lang').value;
	try
	{
		localStorage.setItem('vcmi-terrain-painter-lang', state.lang);
	}
	catch (error)
	{
		console.warn('Language not saved', error);
	}
	applyLanguage();
});
new ResizeObserver(() => requestRender()).observe(view);

// ---------- picture conversion ----------

const converter = {
	image: null,
	clusters: null,
	sampleScale: 1,
	cellCounts: null,
	cellSamples: null,
	background: null,
};

const linearTable = new Float32Array(256);
for (let i = 0; i < 256; i++)
{
	const c = i / 255;
	linearTable[i] = c <= 0.04045 ? c / 12.92 : Math.pow((c + 0.055) / 1.055, 2.4);
}

function toOklab(r, g, b, out, offset)
{
	const lr = linearTable[r];
	const lg = linearTable[g];
	const lb = linearTable[b];
	const l = Math.cbrt(0.4122214708 * lr + 0.5363325363 * lg + 0.0514459929 * lb);
	const m = Math.cbrt(0.2119034982 * lr + 0.6806995451 * lg + 0.1073969566 * lb);
	const s = Math.cbrt(0.0883024619 * lr + 0.2817188376 * lg + 0.6299787005 * lb);
	out[offset] = 0.2104542553 * l + 0.793617785 * m - 0.0040720468 * s;
	out[offset + 1] = 1.9779984951 * l - 2.428592205 * m + 0.4505937099 * s;
	out[offset + 2] = 0.0259040371 * l + 0.7827717662 * m - 0.808675766 * s;
}

function mulberry32(seed)
{
	return () =>
	{
		seed |= 0;
		seed = (seed + 0x6d2b79f5) | 0;
		let value = Math.imul(seed ^ (seed >>> 15), 1 | seed);
		value = (value + Math.imul(value ^ (value >>> 7), 61 | value)) ^ value;
		return ((value ^ (value >>> 14)) >>> 0) / 4294967296;
	};
}

function fitRect(sourceWidth, sourceHeight, width, height, mode)
{
	if (mode === 'stretch')
		return { x: 0, y: 0, width, height };
	const scale = mode === 'cover' ? Math.max(width / sourceWidth, height / sourceHeight) : Math.min(width / sourceWidth, height / sourceHeight);
	const w = sourceWidth * scale;
	const h = sourceHeight * scale;
	return { x: (width - w) / 2, y: (height - h) / 2, width: w, height: h };
}

/// Nearest terrain among typical picture colours
function suggestTerrain(lab)
{
	let best = SUGGESTIONS[0];
	let bestDistance = Infinity;
	const sample = new Float32Array(3);
	for (const suggestion of SUGGESTIONS)
	{
		toOklab(suggestion[0][0], suggestion[0][1], suggestion[0][2], sample, 0);
		const distance = (sample[0] - lab[0]) ** 2 + (sample[1] - lab[1]) ** 2 + (sample[2] - lab[2]) ** 2;
		if (distance < bestDistance)
		{
			bestDistance = distance;
			best = suggestion;
		}
	}
	return { terrain: TERRAINS.findIndex((t) => t.id === best[1]), forest: !!best[2] };
}

let conversionTimer = 0;
function scheduleConversion()
{
	clearTimeout(conversionTimer);
	setStatus(t('converting'));
	conversionTimer = setTimeout(convert, 60);
}

/// Classifies the picture at several samples per cell, so that colour mixing at borders does not create a third terrain
function convert()
{
	const image = converter.image;
	if (!image)
		return;
	const { width, height } = requestedSize();
	const scale = Math.max(1, Math.min(6, Math.floor(1100 / Math.max(width, height))));
	const sw = width * scale;
	const sh = height * scale;
	const canvas = document.createElement('canvas');
	canvas.width = sw;
	canvas.height = sh;
	const context = canvas.getContext('2d', { willReadFrequently: true });
	context.imageSmoothingEnabled = true;
	context.imageSmoothingQuality = 'high';
	const rect = fitRect(image.width, image.height, sw, sh, $('fitMode').value);
	context.drawImage(image, rect.x, rect.y, rect.width, rect.height);
	const pixels = context.getImageData(0, 0, sw, sh).data;

	const total = sw * sh;
	const lab = new Float32Array(total * 3);
	const inside = new Uint8Array(total);
	for (let y = 0; y < sh; y++)
		for (let x = 0; x < sw; x++)
		{
			const i = y * sw + x;
			if (x + 0.5 < rect.x || x + 0.5 > rect.x + rect.width || y + 0.5 < rect.y || y + 0.5 > rect.y + rect.height)
				continue;
			inside[i] = 1;
			toOklab(pixels[i * 4], pixels[i * 4 + 1], pixels[i * 4 + 2], lab, i * 3);
		}

	const k = Number($('clusterCount').value);
	const random = mulberry32(12345);
	const insideIndices = [];
	for (let i = 0; i < total; i++)
		if (inside[i])
			insideIndices.push(i);
	const sampleCount = Math.min(insideIndices.length, 24000);
	const samples = new Float32Array(sampleCount * 3);
	for (let s = 0; s < sampleCount; s++)
	{
		const i = insideIndices[Math.floor(random() * insideIndices.length)];
		samples.set(lab.subarray(i * 3, i * 3 + 3), s * 3);
	}

	// k-means++ seeding, then Lloyd iterations on the samples
	const centers = new Float32Array(k * 3);
	const nearest = new Float32Array(sampleCount).fill(Infinity);
	let first = Math.floor(random() * sampleCount);
	centers.set(samples.subarray(first * 3, first * 3 + 3), 0);
	for (let c = 1; c < k; c++)
	{
		let sum = 0;
		for (let s = 0; s < sampleCount; s++)
		{
			const d = (samples[s * 3] - centers[(c - 1) * 3]) ** 2 + (samples[s * 3 + 1] - centers[(c - 1) * 3 + 1]) ** 2 + (samples[s * 3 + 2] - centers[(c - 1) * 3 + 2]) ** 2;
			if (d < nearest[s])
				nearest[s] = d;
			sum += nearest[s];
		}
		let target = random() * sum;
		let chosen = sampleCount - 1;
		for (let s = 0; s < sampleCount; s++)
		{
			target -= nearest[s];
			if (target <= 0)
			{
				chosen = s;
				break;
			}
		}
		centers.set(samples.subarray(chosen * 3, chosen * 3 + 3), c * 3);
	}
	const closest = (data, offset) =>
	{
		let best = 0;
		let bestDistance = Infinity;
		for (let c = 0; c < k; c++)
		{
			const d = (data[offset] - centers[c * 3]) ** 2 + (data[offset + 1] - centers[c * 3 + 1]) ** 2 + (data[offset + 2] - centers[c * 3 + 2]) ** 2;
			if (d < bestDistance)
			{
				bestDistance = d;
				best = c;
			}
		}
		return best;
	};
	for (let iteration = 0; iteration < 12; iteration++)
	{
		const sums = new Float64Array(k * 4);
		for (let s = 0; s < sampleCount; s++)
		{
			const c = closest(samples, s * 3);
			sums[c * 4] += samples[s * 3];
			sums[c * 4 + 1] += samples[s * 3 + 1];
			sums[c * 4 + 2] += samples[s * 3 + 2];
			sums[c * 4 + 3] += 1;
		}
		for (let c = 0; c < k; c++)
			if (sums[c * 4 + 3] > 0)
				for (let j = 0; j < 3; j++)
					centers[c * 3 + j] = sums[c * 4 + j] / sums[c * 4 + 3];
	}

	// per cell: how many samples fall into each cluster; slot k counts margins outside the picture
	const slots = k + 1;
	const cellCounts = new Uint16Array(width * height * slots);
	const clusterPixels = new Float64Array(k);
	const clusterRgb = new Float64Array(k * 3);
	for (let y = 0; y < sh; y++)
		for (let x = 0; x < sw; x++)
		{
			const i = y * sw + x;
			const cell = Math.floor(y / scale) * width + Math.floor(x / scale);
			const c = inside[i] ? closest(lab, i * 3) : k;
			cellCounts[cell * slots + c]++;
			if (c < k)
			{
				clusterPixels[c]++;
				clusterRgb[c * 3] += pixels[i * 4];
				clusterRgb[c * 3 + 1] += pixels[i * 4 + 1];
				clusterRgb[c * 3 + 2] += pixels[i * 4 + 2];
			}
		}

	const previous = converter.clusters;
	const insideTotal = clusterPixels.reduce((a, b) => a + b, 0) || 1;
	const clusters = [];
	for (let c = 0; c < k; c++)
	{
		const center = centers.subarray(c * 3, c * 3 + 3);
		const suggestion = suggestTerrain(center);
		const rgb = clusterPixels[c] ? [0, 1, 2].map((j) => Math.round(clusterRgb[c * 3 + j] / clusterPixels[c])) : [0, 0, 0];
		const kept = previous?.find((p) => Math.abs(p.lab[0] - center[0]) + Math.abs(p.lab[1] - center[1]) + Math.abs(p.lab[2] - center[2]) < 0.03);
		clusters.push({
			lab: Array.from(center), rgb, share: clusterPixels[c] / insideTotal,
			terrain: kept ? kept.terrain : suggestion.terrain, forest: kept ? kept.forest : suggestion.forest,
		});
	}
	const order = clusters.map((_, c) => c).sort((a, b) => clusters[b].share - clusters[a].share);
	converter.clusters = clusters;
	converter.order = order;
	converter.cellCounts = cellCounts;
	converter.cellSamples = scale * scale;
	converter.width = width;
	converter.height = height;
	renderClusterRows();
	updatePreview();
}

function renderClusterRows()
{
	const rows = $('clusterRows');
	rows.replaceChildren();
	for (const c of converter.order)
	{
		const cluster = converter.clusters[c];
		if (cluster.share < 0.0005)
			continue;
		const row = document.createElement('tr');
		const colorCell = document.createElement('td');
		colorCell.append(swatch(cluster.rgb));
		const shareCell = document.createElement('td');
		shareCell.textContent = `${(cluster.share * 100).toFixed(cluster.share < 0.1 ? 1 : 0)}%`;
		const terrainCell = document.createElement('td');
		const select = document.createElement('select');
		TERRAINS.forEach((_, index) => select.append(new Option(terrainName(index), index)));
		select.value = cluster.terrain;
		select.addEventListener('change', () =>
		{
			cluster.terrain = Number(select.value);
			updatePreview();
		});
		terrainCell.append(select);
		const forestCell = document.createElement('td');
		const forest = document.createElement('input');
		forest.type = 'checkbox';
		forest.checked = cluster.forest;
		forest.addEventListener('change', () =>
		{
			cluster.forest = forest.checked;
			updatePreview();
		});
		forestCell.append(forest);
		row.append(colorCell, shareCell, terrainCell, forestCell);
		rows.append(row);
	}
}

function neighbours(x, y, width, height, toroidal, visit)
{
	for (let dy = -1; dy <= 1; dy++)
		for (let dx = -1; dx <= 1; dx++)
		{
			if (!dx && !dy)
				continue;
			let nx = x + dx;
			let ny = y + dy;
			if (toroidal)
			{
				nx = (nx + width) % width;
				ny = (ny + height) % height;
			}
			else if (nx < 0 || ny < 0 || nx >= width || ny >= height)
				continue;
			visit(ny * width + nx, dx === 0 || dy === 0);
		}
}

/// Replaces each cell by the terrain that holds most of its 3×3 neighbourhood
function majorityPass(grid, width, height, toroidal, values)
{
	const result = grid.slice();
	const counts = new Uint16Array(values);
	for (let y = 0; y < height; y++)
		for (let x = 0; x < width; x++)
		{
			counts.fill(0);
			const index = y * width + x;
			counts[grid[index]]++;
			neighbours(x, y, width, height, toroidal, (n) => counts[grid[n]]++);
			let best = grid[index];
			for (let v = 0; v < values; v++)
				if (counts[v] > counts[best])
					best = v;
			if (counts[best] >= 5)
				result[index] = best;
		}
	return result;
}

/// Merges 4-connected regions smaller than `minimum` cells into their most common neighbour
function removeSmallRegions(grid, width, height, toroidal, minimum)
{
	if (minimum <= 1)
		return grid;
	const labels = new Int32Array(width * height).fill(-1);
	const result = grid.slice();
	let label = 0;
	for (let start = 0; start < grid.length; start++)
	{
		if (labels[start] >= 0)
			continue;
		const region = [start];
		labels[start] = label;
		const around = new Map();
		for (let r = 0; r < region.length; r++)
		{
			const index = region[r];
			const x = index % width;
			const y = (index - x) / width;
			neighbours(x, y, width, height, toroidal, (n, orthogonal) =>
			{
				if (!orthogonal)
					return;
				if (grid[n] === grid[start])
				{
					if (labels[n] < 0)
					{
						labels[n] = label;
						region.push(n);
					}
				}
				else
					around.set(grid[n], (around.get(grid[n]) ?? 0) + 1);
			});
		}
		if (region.length < minimum && around.size)
		{
			const replacement = [...around.entries()].sort((a, b) => b[1] - a[1])[0][0];
			for (const index of region)
				result[index] = replacement;
		}
		label++;
	}
	return result;
}

function updatePreview()
{
	const { width, height, cellCounts, clusters } = converter;
	const k = clusters.length;
	const slots = k + 1;
	const fill = Number($('fillTerrain').value);
	let terrain = new Uint8Array(width * height);
	let obstacles = new Uint8Array(width * height);
	const votes = new Float64Array(TERRAINS.length);
	for (let cell = 0; cell < width * height; cell++)
	{
		votes.fill(0);
		let forest = 0;
		for (let c = 0; c < k; c++)
		{
			const count = cellCounts[cell * slots + c];
			votes[clusters[c].terrain] += count;
			if (clusters[c].forest)
				forest += count;
		}
		votes[fill] += cellCounts[cell * slots + k];
		let best = 0;
		for (let v = 1; v < TERRAINS.length; v++)
			if (votes[v] > votes[best])
				best = v;
		terrain[cell] = best;
		obstacles[cell] = forest * 2 > converter.cellSamples ? 1 : 0;
	}
	const toroidal = $('toroidal').checked;
	for (let pass = 0; pass < Number($('smoothing').value); pass++)
	{
		terrain = majorityPass(terrain, width, height, toroidal, TERRAINS.length);
		obstacles = majorityPass(obstacles, width, height, toroidal, 2);
	}
	terrain = removeSmallRegions(terrain, width, height, toroidal, Number($('minRegion').value));
	for (let i = 0; i < terrain.length; i++)
		if (TERRAINS[terrain[i]].id === 'water' || terrain[i] === ROCK)
			obstacles[i] = 0;

	const resized = !state.preview || state.preview.width !== width || state.preview.height !== height;
	state.preview = { width, height, terrain, obstacles };
	cellsDirty = true;
	setStatus(t('previewActive'));
	if (resized)
		fitView();
	requestRender();
}

function loadPicture(file)
{
	if (!file || !file.type.startsWith('image/'))
		return;
	const url = URL.createObjectURL(file);
	const image = new Image();
	image.onload = () =>
	{
		converter.image = image;
		converter.clusters = null;
		underlay = image;
		$('converter').hidden = false;
		convert();
	};
	image.onerror = () =>
	{
		URL.revokeObjectURL(url);
		setStatus(t('imageFailed'), true);
	};
	image.src = url;
}

$('imageInput').addEventListener('change', () => loadPicture($('imageInput').files[0]));
const dropZone = $('dropZone');
dropZone.addEventListener('dragover', (event) =>
{
	event.preventDefault();
	dropZone.classList.add('over');
});
dropZone.addEventListener('dragleave', () => dropZone.classList.remove('over'));
dropZone.addEventListener('drop', (event) =>
{
	event.preventDefault();
	dropZone.classList.remove('over');
	loadPicture(event.dataTransfer.files[0]);
});
dropZone.addEventListener('keydown', (event) =>
{
	if (event.key === 'Enter' || event.key === ' ')
		$('imageInput').click();
});
window.addEventListener('paste', (event) =>
{
	const item = [...(event.clipboardData?.items ?? [])].find((i) => i.type.startsWith('image/'));
	if (item)
		loadPicture(item.getAsFile());
});
$('fitMode').addEventListener('change', () =>
{
	scheduleConversion();
	requestRender();
});
$('clusterCount').addEventListener('input', () =>
{
	$('clusterCountOut').textContent = $('clusterCount').value;
	scheduleConversion();
});
for (const id of ['smoothing', 'minRegion'])
	$(id).addEventListener('input', () =>
	{
		$(`${id}Out`).textContent = $(id).value;
		if (converter.clusters)
			updatePreview();
	});
$('fillTerrain').addEventListener('change', () =>
{
	if (converter.clusters)
		updatePreview();
});
$('matchAspect').addEventListener('click', () =>
{
	if (!converter.image)
		return;
	const { width } = requestedSize();
	$('mapHeight').value = Math.max(8, Math.min(MAX_SIDE, Math.round((width * converter.image.height) / converter.image.width)));
	$('sizePreset').value = 'custom';
	scheduleConversion();
});
$('applyConversion').addEventListener('click', () =>
{
	const preview = state.preview;
	if (!preview)
		return;
	state.preview = null;
	commitWhole(() =>
	{
		state.width = preview.width;
		state.height = preview.height;
		state.terrain = preview.terrain;
		state.obstacles = preview.obstacles;
	});
	syncSizeInputs();
	$('converter').hidden = true;
	setStatus('');
});
$('discardConversion').addEventListener('click', () =>
{
	state.preview = null;
	$('converter').hidden = true;
	cellsDirty = true;
	setStatus('');
	fitView();
});

// ---------- files ----------

function download(blob, name)
{
	const url = URL.createObjectURL(blob);
	const link = document.createElement('a');
	link.href = url;
	link.download = name;
	document.body.append(link);
	link.click();
	link.remove();
	setTimeout(() => URL.revokeObjectURL(url), 2000);
}

function baseFileName()
{
	const name = ($('mapName').value.trim() || t('defaultName')).replace(/[\\/:*?"<>|]+/g, '').trim();
	return name || 'map';
}

$('exportPng').addEventListener('click', () =>
{
	const canvas = document.createElement('canvas');
	canvas.width = state.width;
	canvas.height = state.height;
	const context = canvas.getContext('2d');
	const image = context.createImageData(state.width, state.height);
	for (let i = 0; i < state.terrain.length; i++)
	{
		const terrain = TERRAINS[state.terrain[i]];
		const color = state.obstacles[i] ? terrain.blocked : terrain.color;
		image.data.set([...color, 255], i * 4);
	}
	context.putImageData(image, 0, 0);
	canvas.toBlob((blob) => download(blob, `${baseFileName()}.png`), 'image/png');
});

$('importPng').addEventListener('click', () => $('pngInput').click());
function loadObjectsOverlay(file)
{
	const url = URL.createObjectURL(file);
	const image = new Image();
	image.onload = () =>
	{
		URL.revokeObjectURL(url);
		objectsOverlay = image;
		$('showObjectsRow').hidden = false;
		if (image.width !== state.width || image.height !== state.height)
			setStatus(t('objectsSizeMismatch', { w: image.width, h: image.height, mw: state.width, mh: state.height }), true);
		requestRender();
	};
	image.onerror = () =>
	{
		URL.revokeObjectURL(url);
		setStatus(t('imageFailed'), true);
	};
	image.src = url;
}

$('pngInput').addEventListener('change', () =>
{
	const files = [...$('pngInput').files];
	$('pngInput').value = '';
	// the VCMI map editor saves objects next to the terrain as NAME-objects.png
	const objects = files.find((f) => /-objects\.png$/i.test(f.name));
	const file = files.find((f) => f !== objects);
	if (objects && !file)
		loadObjectsOverlay(objects);
	if (!file)
		return;
	const url = URL.createObjectURL(file);
	const image = new Image();
	image.onload = () =>
	{
		URL.revokeObjectURL(url);
		if (image.width > MAX_SIDE || image.height > MAX_SIDE)
		{
			setStatus(t('pngTooLarge', { max: MAX_SIDE }), true);
			return;
		}
		const canvas = document.createElement('canvas');
		canvas.width = image.width;
		canvas.height = image.height;
		const context = canvas.getContext('2d', { willReadFrequently: true });
		context.drawImage(image, 0, 0);
		const pixels = context.getImageData(0, 0, image.width, image.height).data;
		const colors = [];
		TERRAINS.forEach((terrain, index) =>
		{
			colors.push([terrain.color, index, 0]);
			if (terrain.blocked.join() !== terrain.color.join())
				colors.push([terrain.blocked, index, 1]);
		});
		commitWhole(() =>
		{
			state.width = image.width;
			state.height = image.height;
			state.terrain = new Uint8Array(image.width * image.height);
			state.obstacles = new Uint8Array(image.width * image.height);
			for (let i = 0; i < state.terrain.length; i++)
			{
				let best = colors[0];
				let bestDistance = Infinity;
				for (const entry of colors)
				{
					const d = (entry[0][0] - pixels[i * 4]) ** 2 + (entry[0][1] - pixels[i * 4 + 1]) ** 2 + (entry[0][2] - pixels[i * 4 + 2]) ** 2;
					if (d < bestDistance)
					{
						bestDistance = d;
						best = entry;
					}
				}
				state.terrain[i] = best[1];
				state.obstacles[i] = best[2];
			}
		});
		syncSizeInputs();
		if (objects)
			loadObjectsOverlay(objects);
	};
	image.onerror = () =>
	{
		URL.revokeObjectURL(url);
		setStatus(t('imageFailed'), true);
	};
	image.src = url;
});

$('exportPalette').addEventListener('click', () =>
{
	const lines = ['GIMP Palette', 'Name: VCMI terrain', 'Columns: 2', '#'];
	TERRAINS.forEach((terrain, index) =>
	{
		lines.push(`${terrain.color.join(' ')}\t${terrainName(index)}`);
		if (terrain.blocked.join() !== terrain.color.join())
			lines.push(`${terrain.blocked.join(' ')}\t${terrainName(index)} — ${t('withObstacles')}`);
	});
	download(new Blob([lines.join('\n') + '\n'], { type: 'text/plain' }), 'vcmi-terrain.gpl');
});

// ---------- building ----------

function buildRequest()
{
	const legend = {};
	const terrainRows = [];
	const obstacleRows = [];
	for (let y = 0; y < state.height; y++)
	{
		let terrainRow = '';
		let obstacleRow = '';
		for (let x = 0; x < state.width; x++)
		{
			const index = y * state.width + x;
			const terrain = TERRAINS[state.terrain[index]];
			legend[terrain.letter] = terrain.id;
			terrainRow += terrain.letter;
			obstacleRow += state.obstacles[index] ? 'x' : '.';
		}
		terrainRows.push(terrainRow);
		obstacleRows.push(obstacleRow);
	}
	return {
		name: $('mapName').value.trim() || t('defaultName'),
		description: $('mapDescription').value.trim(),
		toroidal: state.toroidal,
		seed: Math.floor(Math.random() * 2147483647),
		legend,
		levels: [{ terrain: terrainRows, obstacles: obstacleRows }],
	};
}

$('buildMap').addEventListener('click', async () =>
{
	if (state.preview)
	{
		setStatus(t('previewActive'));
		return;
	}
	if (state.terrain.some((terrain) => TERRAINS[terrain].hota))
	{
		setStatus(t('hotaBuild'), true);
		return;
	}
	const button = $('buildMap');
	button.disabled = true;
	setStatus(t('building'));
	try
	{
		const response = await fetch(API_URL, {
			method: 'POST',
			headers: { 'Content-Type': 'application/json' },
			body: JSON.stringify(buildRequest()),
		});
		const reply = await response.json().catch(() => ({ error: `HTTP ${response.status}` }));
		if (!response.ok || reply.error)
			throw new Error(reply.error || `HTTP ${response.status}`);

		download(new Blob([fromBase64(reply.map)], { type: 'application/octet-stream' }), `${baseFileName()}.vmap`);
		state.changed.clear();
		state.changedTerrain.clear();
		const letters = new Map(TERRAINS.map((terrain, index) => [terrain.letter, index]));
		for (const [x, y, z, letter] of reply.report.changed)
		{
			if (z !== 0 || !letters.has(letter))
				continue;
			const index = y * state.width + x;
			state.changed.add(index);
			state.changedTerrain.set(index, letters.get(letter));
		}
		const values = { obstacles: reply.report.obstacles, changed: reply.report.changed.length };
		$('buildSummary').textContent = values.changed ? t('built', values) : t('builtClean', values);
		$('buildResult').hidden = false;
		$('acceptChanges').hidden = values.changed === 0;
		setStatus('');
	}
	catch (error)
	{
		setStatus(t('buildFailed', { error: error.message }), true);
	}
	finally
	{
		button.disabled = false;
		requestRender();
	}
});

$('acceptChanges').addEventListener('click', () =>
{
	if (!state.changedTerrain.size)
	{
		setStatus(t('nothingToAccept'));
		return;
	}
	const changes = [...state.changedTerrain];
	beginStroke();
	for (const [index, terrain] of changes)
		setCell(index, terrain, state.obstacles[index]);
	endStroke();
	clearChanged();
});

// ---------- start ----------

function start()
{
	let lang = navigator.language?.toLowerCase().startsWith('ru') ? 'ru' : 'en';
	const requested = new URLSearchParams(location.search).get('lang');
	try
	{
		lang = localStorage.getItem('vcmi-terrain-painter-lang') ?? lang;
	}
	catch (error)
	{
		console.warn('Language not restored', error);
	}
	if (requested === 'ru' || requested === 'en')
		lang = requested;
	state.lang = lang;
	$('lang').value = lang;

	const restored = loadDraft();
	$('toroidal').checked = state.toroidal;
	syncSizeInputs();
	applyLanguage();
	selectTerrain(GRASS);
	setBrush(3);
	afterEdit();
	fitView();
	if (restored)
		setStatus(t('savedDraft'));
}

start();
